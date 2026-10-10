#include <gtk/gtk.h>
#include "energy.h"

/*
 * ============================================================================
 * GLOBAL APPLICATION STATE & GUI WIDGET REFERENCES
 * ============================================================================
 */
typedef struct {
    ApplianceList *list;
    double current_rate;
    int current_days;
    int editing_id; /* 0: Adding new appliance; > 0: Updating existing appliance ID */

    /* Window & Top settings */
    GtkWidget *main_window;
    GtkWidget *spin_rate;
    GtkWidget *spin_days;

    /* Input form widgets */
    GtkWidget *lbl_form_mode;
    GtkWidget *entry_name;
    GtkWidget *spin_quantity;
    GtkWidget *entry_power;
    GtkWidget *entry_hours;
    GtkWidget *btn_submit;
    GtkWidget *btn_clear_form;

    /* Search & DSA widgets */
    GtkWidget *entry_search;

    /* Table widgets */
    GtkWidget *tree_view;
    GtkListStore *list_store;

    /* Summary labels */
    GtkWidget *lbl_sum_appliances;
    GtkWidget *lbl_sum_daily;
    GtkWidget *lbl_sum_monthly;
    GtkWidget *lbl_sum_bill;
    GtkWidget *lbl_sum_highest;
} AppWidgets;

static AppWidgets g_app;

/* Enum for TreeView columns */
enum {
    COL_ID = 0,
    COL_NAME,
    COL_QTY,
    COL_POWER_STR,
    COL_HOURS_STR,
    COL_DAILY_STR,
    COL_MONTHLY_STR,
    COL_COST_STR,
    NUM_COLS
};

/* Forward declarations */
static void refresh_gui_state(void);
static void show_error_dialog(const char *message);
static void show_info_dialog(const char *title, const char *message);
static void reset_form_inputs(void);

/*
 * ============================================================================
 * MODERN GTK CSS STYLING
 * ============================================================================
 */
static void apply_custom_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css_data =
        "window { background-color: #f4f6f9; font-family: 'Segoe UI', Arial, sans-serif; }\n"
        "frame { border: 1px solid #dcdfe6; border-radius: 8px; background-color: #ffffff; margin: 4px; padding: 6px; }\n"
        "frame > border { border: 1px solid #e2e8f0; border-radius: 8px; }\n"
        "frame > label { font-weight: bold; color: #1e293b; padding: 0 4px; }\n"
        ".app-header { background: linear-gradient(135deg, #1e3a8a, #0284c7); padding: 12px; border-radius: 6px; color: #ffffff; }\n"
        ".app-title { font-size: 18px; font-weight: bold; color: #ffffff; }\n"
        ".app-subtitle { font-size: 11px; color: #e0f2fe; }\n"
        ".btn-primary { background: #0284c7; color: white; border-radius: 4px; font-weight: bold; padding: 6px 12px; }\n"
        ".btn-primary:hover { background: #0369a1; }\n"
        ".btn-success { background: #16a34a; color: white; border-radius: 4px; font-weight: bold; padding: 6px 12px; }\n"
        ".btn-success:hover { background: #15803d; }\n"
        ".btn-danger { background: #dc2626; color: white; border-radius: 4px; font-weight: bold; padding: 6px 12px; }\n"
        ".btn-danger:hover { background: #b91c1c; }\n"
        ".summary-card { background: #f8fafc; border: 1px solid #cbd5e1; border-radius: 6px; padding: 8px; }\n"
        ".summary-val { font-size: 15px; font-weight: bold; color: #0f172a; }\n"
        ".summary-highlight { font-size: 16px; font-weight: bold; color: #0284c7; }\n"
        ".summary-top { font-size: 14px; font-weight: bold; color: #ea580c; }\n"
        ".pbl-notice { font-size: 10px; color: #64748b; font-style: italic; }\n"
        "treeview { font-size: 12px; }\n"
        "treeview:selected { background-color: #38bdf8; color: #000000; }\n";

    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);
    GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

/*
 * ============================================================================
 * GUI HELPER FUNCTIONS
 * ============================================================================
 */

static void show_error_dialog(const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s",
        message
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "Validation Error");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void show_info_dialog(const char *title, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s",
        message
    );
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void reset_form_inputs(void) {
    g_app.editing_id = 0;
    gtk_label_set_text(GTK_LABEL(g_app.lbl_form_mode), "Mode: Add New Appliance");
    gtk_button_set_label(GTK_BUTTON(g_app.btn_submit), "Add Appliance");

    gtk_entry_set_text(GTK_ENTRY(g_app.entry_name), "");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_quantity), 1.0);
    gtk_entry_set_text(GTK_ENTRY(g_app.entry_power), "");
    gtk_entry_set_text(GTK_ENTRY(g_app.entry_hours), "");
}

/*
 * Synchronizes backend linked list state with the GUI table & summary cards
 */
static void refresh_gui_state(void) {
    /* Read active calculation settings */
    g_app.current_rate = gtk_spin_button_get_value(GTK_SPIN_BUTTON(g_app.spin_rate));
    g_app.current_days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_app.spin_days));

    /* 1. Refresh Table Store */
    gtk_list_store_clear(g_app.list_store);

    Appliance *curr = g_app.list->head;
    while (curr != NULL) {
        GtkTreeIter iter;
        gtk_list_store_append(g_app.list_store, &iter);

        double daily_kwh = calculate_daily_kwh(curr);
        double monthly_kwh = calculate_monthly_kwh(curr, g_app.current_days);
        double cost = calculate_appliance_cost(curr, g_app.current_days, g_app.current_rate);

        char power_str[32];
        char hours_str[32];
        char daily_str[32];
        char monthly_str[32];
        char cost_str[32];

        snprintf(power_str, sizeof(power_str), "%.1f W", curr->power_watts);
        snprintf(hours_str, sizeof(hours_str), "%.1f hrs", curr->hours_per_day);
        snprintf(daily_str, sizeof(daily_str), "%.3f kWh", daily_kwh);
        snprintf(monthly_str, sizeof(monthly_str), "%.2f kWh", monthly_kwh);
        snprintf(cost_str, sizeof(cost_str), "INR %.2f", cost);

        gtk_list_store_set(g_app.list_store, &iter,
                           COL_ID, curr->id,
                           COL_NAME, curr->name,
                           COL_QTY, curr->quantity,
                           COL_POWER_STR, power_str,
                           COL_HOURS_STR, hours_str,
                           COL_DAILY_STR, daily_str,
                           COL_MONTHLY_STR, monthly_str,
                           COL_COST_STR, cost_str,
                           -1);

        curr = curr->next;
    }

    /* 2. Refresh Summary Metrics */
    EnergySummary summary;
    calculate_summary(g_app.list, g_app.current_days, g_app.current_rate, &summary);

    char buf[128];
    snprintf(buf, sizeof(buf), "%d appliances (%d total units)", summary.total_appliances, summary.total_units);
    gtk_label_set_text(GTK_LABEL(g_app.lbl_sum_appliances), buf);

    snprintf(buf, sizeof(buf), "%.3f kWh / day", summary.total_daily_kwh);
    gtk_label_set_text(GTK_LABEL(g_app.lbl_sum_daily), buf);

    snprintf(buf, sizeof(buf), "%.2f kWh / month", summary.total_monthly_kwh);
    gtk_label_set_text(GTK_LABEL(g_app.lbl_sum_monthly), buf);

    snprintf(buf, sizeof(buf), "INR %.2f", summary.estimated_bill);
    gtk_label_set_text(GTK_LABEL(g_app.lbl_sum_bill), buf);

    if (summary.total_appliances > 0 && strcmp(summary.highest_consumer_name, "None") != 0) {
        snprintf(buf, sizeof(buf), "%s (%.3f kWh/day - %.1f%%)",
                 summary.highest_consumer_name,
                 summary.highest_daily_kwh,
                 summary.highest_percentage);
    } else {
        snprintf(buf, sizeof(buf), "None");
    }
    gtk_label_set_text(GTK_LABEL(g_app.lbl_sum_highest), buf);
}

/*
 * ============================================================================
 * BUTTON & USER ACTION CALLBACKS
 * ============================================================================
 */

/* Callback: Submit appliance (Add or Update) */
static void on_btn_submit_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    const char *name = gtk_entry_get_text(GTK_ENTRY(g_app.entry_name));
    int quantity = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_app.spin_quantity));
    const char *power_str = gtk_entry_get_text(GTK_ENTRY(g_app.entry_power));
    const char *hours_str = gtk_entry_get_text(GTK_ENTRY(g_app.entry_hours));

    double power_watts = atof(power_str);
    double hours_per_day = atof(hours_str);

    char err_msg[128];
    if (!validate_appliance_input(name, quantity, power_watts, hours_per_day, err_msg, sizeof(err_msg))) {
        show_error_dialog(err_msg);
        return;
    }

    if (g_app.editing_id == 0) {
        /* Add new appliance */
        if (!add_appliance(g_app.list, name, quantity, power_watts, hours_per_day)) {
            show_error_dialog("Failed to insert appliance into data structure.");
            return;
        }
    } else {
        /* Update existing appliance */
        if (!update_appliance(g_app.list, g_app.editing_id, name, quantity, power_watts, hours_per_day)) {
            show_error_dialog("Failed to update selected appliance node.");
            return;
        }
    }

    reset_form_inputs();
    refresh_gui_state();
}

/* Callback: Clear / cancel form input */
static void on_btn_clear_form_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    reset_form_inputs();
}

/* Callback: Recalculate button / settings change */
static void on_btn_recalc_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    double rate = gtk_spin_button_get_value(GTK_SPIN_BUTTON(g_app.spin_rate));
    int days = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(g_app.spin_days));

    char err_msg[128];
    if (!validate_settings(rate, days, err_msg, sizeof(err_msg))) {
        show_error_dialog(err_msg);
        return;
    }

    refresh_gui_state();
}

/* Helper to get selected appliance ID from TreeView */
static int get_selected_appliance_id(void) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_app.tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        int id = 0;
        gtk_tree_model_get(model, &iter, COL_ID, &id, -1);
        return id;
    }
    return 0;
}

/* Callback: Edit selected row */
static void on_btn_edit_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    int id = get_selected_appliance_id();
    if (id <= 0) {
        show_error_dialog("Please select an appliance from the table to edit.");
        return;
    }

    Appliance *app = search_appliance_by_id(g_app.list, id);
    if (!app) {
        show_error_dialog("Selected appliance not found in linked list.");
        return;
    }

    g_app.editing_id = app->id;

    char mode_text[64];
    snprintf(mode_text, sizeof(mode_text), "Mode: Editing Appliance #%d", app->id);
    gtk_label_set_text(GTK_LABEL(g_app.lbl_form_mode), mode_text);
    gtk_button_set_label(GTK_BUTTON(g_app.btn_submit), "Save Changes");

    gtk_entry_set_text(GTK_ENTRY(g_app.entry_name), app->name);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_quantity), (double)app->quantity);

    char num_buf[32];
    snprintf(num_buf, sizeof(num_buf), "%.2f", app->power_watts);
    gtk_entry_set_text(GTK_ENTRY(g_app.entry_power), num_buf);

    snprintf(num_buf, sizeof(num_buf), "%.2f", app->hours_per_day);
    gtk_entry_set_text(GTK_ENTRY(g_app.entry_hours), num_buf);
}

/* Callback: Delete selected row */
static void on_btn_delete_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    int id = get_selected_appliance_id();
    if (id <= 0) {
        show_error_dialog("Please select an appliance from the table to delete.");
        return;
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Are you sure you want to delete appliance #%d?",
        id
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirm Delete");

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (result == GTK_RESPONSE_YES) {
        if (g_app.editing_id == id) {
            reset_form_inputs();
        }
        delete_appliance(g_app.list, id);
        refresh_gui_state();
    }
}

/* Callback: Search appliances (Linear Search by ID or Name) */
static void on_btn_search_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    const char *query = gtk_entry_get_text(GTK_ENTRY(g_app.entry_search));
    if (!query || strlen(query) == 0) {
        show_error_dialog("Please enter an appliance name or numeric ID to search.");
        return;
    }

    Appliance *found = NULL;
    int search_id = atoi(query);
    if (search_id > 0) {
        found = search_appliance_by_id(g_app.list, search_id);
    }
    if (!found) {
        found = search_appliance_by_name(g_app.list, query);
    }

    if (found != NULL) {
        /* Select matching row in the TreeView */
        GtkTreeModel *model = GTK_TREE_MODEL(g_app.list_store);
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

        while (valid) {
            int row_id = 0;
            gtk_tree_model_get(model, &iter, COL_ID, &row_id, -1);
            if (row_id == found->id) {
                GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_app.tree_view));
                gtk_tree_selection_select_iter(selection, &iter);
                GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(g_app.tree_view), path, NULL, TRUE, 0.5, 0.0);
                gtk_tree_path_free(path);
                break;
            }
            valid = gtk_tree_model_iter_next(model, &iter);
        }

        char info[256];
        double daily = calculate_daily_kwh(found);
        double monthly = calculate_monthly_kwh(found, g_app.current_days);
        double cost = calculate_appliance_cost(found, g_app.current_days, g_app.current_rate);

        snprintf(info, sizeof(info),
                 "Found Appliance:\n\n"
                 "ID: %d\n"
                 "Name: %s\n"
                 "Quantity: %d\n"
                 "Power: %.1f W\n"
                 "Usage: %.1f hours/day\n"
                 "Daily Consumption: %.3f kWh\n"
                 "Monthly Consumption: %.2f kWh\n"
                 "Estimated Cost: INR %.2f",
                 found->id, found->name, found->quantity,
                 found->power_watts, found->hours_per_day,
                 daily, monthly, cost);

        show_info_dialog("Search Result", info);
    } else {
        char err[128];
        snprintf(err, sizeof(err), "No appliance found matching '%s'.", query);
        show_error_dialog(err);
    }
}

/* Callback: Sort appliances descending by energy consumption using DSA Merge Sort */
static void on_btn_sort_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    if (!g_app.list || g_app.list->count <= 1) {
        show_info_dialog("Sort Info", "Appliance list has 1 or fewer items. Nothing to sort.");
        return;
    }

    /* Execute manual O(N log N) recursive Merge Sort on singly linked list */
    sort_appliances_by_consumption(g_app.list, g_app.current_days);
    refresh_gui_state();

    show_info_dialog("Appliances Sorted",
                     "Appliances have been sorted in descending order of energy consumption\n"
                     "using an in-place Recursive Merge Sort algorithm [O(N log N)].");
}

/* Callback: Save data to data.txt */
static void on_btn_save_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    if (save_appliances_to_file(g_app.list, DEFAULT_DATA_FILE, g_app.current_rate, g_app.current_days)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Successfully saved %d appliances to %s.", g_app.list->count, DEFAULT_DATA_FILE);
        show_info_dialog("Data Saved", msg);
    } else {
        show_error_dialog("Failed to save data to file data.txt.");
    }
}

/* Callback: Reload data from data.txt */
static void on_btn_reload_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    double loaded_rate = g_app.current_rate;
    int loaded_days = g_app.current_days;

    if (load_appliances_from_file(g_app.list, DEFAULT_DATA_FILE, &loaded_rate, &loaded_days)) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_rate), loaded_rate);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_days), (double)loaded_days);
        reset_form_inputs();
        refresh_gui_state();

        char msg[128];
        snprintf(msg, sizeof(msg), "Loaded %d appliances from %s.", g_app.list->count, DEFAULT_DATA_FILE);
        show_info_dialog("Data Reloaded", msg);
    } else {
        show_error_dialog("Could not read from data.txt or file is empty.");
    }
}

/* Callback: Clear all appliances */
static void on_btn_clear_all_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g_app.main_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_YES_NO,
        "Are you sure you want to clear all entered appliances? This action cannot be undone."
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "Clear All Appliances");

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (result == GTK_RESPONSE_YES) {
        clear_all_appliances(g_app.list);
        reset_form_inputs();
        refresh_gui_state();
    }
}

/* Callback: Table row double-click activates edit mode */
static void on_tree_view_row_activated(GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata) {
    (void)treeview;
    (void)path;
    (void)col;
    (void)userdata;
    on_btn_edit_clicked(NULL, NULL);
}

/*
 * ============================================================================
 * GUI LAYOUT BUILDER
 * ============================================================================
 */

static GtkWidget* create_table_column(const char *title, int col_id) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(title, renderer, "text", col_id, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_sort_column_id(col, col_id);
    return (GtkWidget*)col;
}

static void build_gui(void) {
    /* 1. Main Window */
    g_app.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_app.main_window), "Smart Energy Consumption Tracker [Desktop C & GTK]");
    gtk_window_set_default_size(GTK_WINDOW(g_app.main_window), 1080, 720);
    gtk_window_set_position(GTK_WINDOW(g_app.main_window), GTK_WIN_POS_CENTER);
    g_signal_connect(g_app.main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 10);
    gtk_container_add(GTK_CONTAINER(g_app.main_window), main_vbox);

    /* 2. Header Banner */
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkStyleContext *header_ctx = gtk_widget_get_style_context(header_box);
    gtk_style_context_add_class(header_ctx, "app-header");

    GtkWidget *title_lbl = gtk_label_new("⚡ SMART ENERGY CONSUMPTION TRACKER");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);
    GtkStyleContext *title_ctx = gtk_widget_get_style_context(title_lbl);
    gtk_style_context_add_class(title_ctx, "app-title");
    gtk_box_pack_start(GTK_BOX(header_box), title_lbl, FALSE, FALSE, 0);

    GtkWidget *sub_lbl = gtk_label_new("Academic Data Structures Project • Singly Linked List • In-Place Merge Sort • File Persistence");
    gtk_widget_set_halign(sub_lbl, GTK_ALIGN_START);
    GtkStyleContext *sub_ctx = gtk_widget_get_style_context(sub_lbl);
    gtk_style_context_add_class(sub_ctx, "app-subtitle");
    gtk_box_pack_start(GTK_BOX(header_box), sub_lbl, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), header_box, FALSE, FALSE, 0);

    /* 3. Top Settings Bar */
    GtkWidget *settings_frame = gtk_frame_new("Calculation Settings");
    GtkWidget *settings_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(settings_box), 6);

    GtkWidget *lbl_rate = gtk_label_new("Electricity Rate (INR/kWh):");
    g_app.spin_rate = gtk_spin_button_new_with_range(0.1, 100.0, 0.25);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_rate), DEFAULT_RATE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g_app.spin_rate), 2);

    GtkWidget *lbl_days = gtk_label_new("Billing Period (Days):");
    g_app.spin_days = gtk_spin_button_new_with_range(1.0, 365.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_days), DEFAULT_DAYS);

    GtkWidget *btn_recalc = gtk_button_new_with_label("Recalculate");
    GtkStyleContext *recalc_ctx = gtk_widget_get_style_context(btn_recalc);
    gtk_style_context_add_class(recalc_ctx, "btn-primary");
    g_signal_connect(btn_recalc, "clicked", G_CALLBACK(on_btn_recalc_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(settings_box), lbl_rate, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(settings_box), g_app.spin_rate, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(settings_box), lbl_days, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(settings_box), g_app.spin_days, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(settings_box), btn_recalc, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(settings_frame), settings_box);
    gtk_box_pack_start(GTK_BOX(main_vbox), settings_frame, FALSE, FALSE, 0);

    /* 4. Split Layout (Left Panel: Inputs & DSA Tools; Right Panel: Table & Summary) */
    GtkWidget *content_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), content_hbox, TRUE, TRUE, 0);

    /* --- Left Panel --- */
    GtkWidget *left_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_size_request(left_panel, 330, -1);
    gtk_box_pack_start(GTK_BOX(content_hbox), left_panel, FALSE, FALSE, 0);

    /* A. Appliance Entry Card */
    GtkWidget *form_frame = gtk_frame_new("Appliance Input");
    GtkWidget *form_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(form_vbox), 8);

    g_app.lbl_form_mode = gtk_label_new("Mode: Add New Appliance");
    gtk_widget_set_halign(g_app.lbl_form_mode, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(form_vbox), g_app.lbl_form_mode, FALSE, FALSE, 0);

    GtkWidget *grid_inputs = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_inputs), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid_inputs), 8);

    /* Name */
    GtkWidget *lbl_name = gtk_label_new("Name:");
    gtk_widget_set_halign(lbl_name, GTK_ALIGN_END);
    g_app.entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_app.entry_name), "e.g., Air Conditioner");
    gtk_grid_attach(GTK_GRID(grid_inputs), lbl_name, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_inputs), g_app.entry_name, 1, 0, 1, 1);

    /* Quantity */
    GtkWidget *lbl_qty = gtk_label_new("Quantity:");
    gtk_widget_set_halign(lbl_qty, GTK_ALIGN_END);
    g_app.spin_quantity = gtk_spin_button_new_with_range(1.0, 500.0, 1.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_quantity), 1.0);
    gtk_grid_attach(GTK_GRID(grid_inputs), lbl_qty, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_inputs), g_app.spin_quantity, 1, 1, 1, 1);

    /* Power */
    GtkWidget *lbl_power = gtk_label_new("Power (Watts):");
    gtk_widget_set_halign(lbl_power, GTK_ALIGN_END);
    g_app.entry_power = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_app.entry_power), "e.g., 1500");
    gtk_grid_attach(GTK_GRID(grid_inputs), lbl_power, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_inputs), g_app.entry_power, 1, 2, 1, 1);

    /* Hours */
    GtkWidget *lbl_hours = gtk_label_new("Hours / Day:");
    gtk_widget_set_halign(lbl_hours, GTK_ALIGN_END);
    g_app.entry_hours = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_app.entry_hours), "e.g., 6.0 (max 24)");
    gtk_grid_attach(GTK_GRID(grid_inputs), lbl_hours, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_inputs), g_app.entry_hours, 1, 3, 1, 1);

    gtk_box_pack_start(GTK_BOX(form_vbox), grid_inputs, FALSE, FALSE, 0);

    /* Form buttons */
    GtkWidget *form_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    g_app.btn_submit = gtk_button_new_with_label("Add Appliance");
    GtkStyleContext *submit_ctx = gtk_widget_get_style_context(g_app.btn_submit);
    gtk_style_context_add_class(submit_ctx, "btn-success");
    g_signal_connect(g_app.btn_submit, "clicked", G_CALLBACK(on_btn_submit_clicked), NULL);

    g_app.btn_clear_form = gtk_button_new_with_label("Clear Form");
    g_signal_connect(g_app.btn_clear_form, "clicked", G_CALLBACK(on_btn_clear_form_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(form_btn_box), g_app.btn_submit, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(form_btn_box), g_app.btn_clear_form, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(form_vbox), form_btn_box, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(form_frame), form_vbox);
    gtk_box_pack_start(GTK_BOX(left_panel), form_frame, FALSE, FALSE, 0);

    /* B. DSA Search, Sort & Persistence Controls */
    GtkWidget *dsa_frame = gtk_frame_new("DSA Operations & Storage");
    GtkWidget *dsa_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(dsa_vbox), 8);

    /* Search Box */
    GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    g_app.entry_search = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_app.entry_search), "Search Name or ID...");
    GtkWidget *btn_search = gtk_button_new_with_label("Search");
    g_signal_connect(btn_search, "clicked", G_CALLBACK(on_btn_search_clicked), NULL);
    g_signal_connect(g_app.entry_search, "activate", G_CALLBACK(on_btn_search_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(search_box), g_app.entry_search, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), btn_search, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dsa_vbox), search_box, FALSE, FALSE, 0);

    /* Sort Button */
    GtkWidget *btn_sort = gtk_button_new_with_label("Sort by Energy (Merge Sort)");
    GtkStyleContext *sort_ctx = gtk_widget_get_style_context(btn_sort);
    gtk_style_context_add_class(sort_ctx, "btn-primary");
    g_signal_connect(btn_sort, "clicked", G_CALLBACK(on_btn_sort_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(dsa_vbox), btn_sort, FALSE, FALSE, 0);

    /* File Storage Buttons */
    GtkWidget *file_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *btn_save = gtk_button_new_with_label("Save (data.txt)");
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_btn_save_clicked), NULL);

    GtkWidget *btn_reload = gtk_button_new_with_label("Reload File");
    g_signal_connect(btn_reload, "clicked", G_CALLBACK(on_btn_reload_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(file_btn_box), btn_save, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(file_btn_box), btn_reload, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(dsa_vbox), file_btn_box, FALSE, FALSE, 0);

    /* Clear All Button */
    GtkWidget *btn_clear_all = gtk_button_new_with_label("Clear All Appliances");
    GtkStyleContext *clear_all_ctx = gtk_widget_get_style_context(btn_clear_all);
    gtk_style_context_add_class(clear_all_ctx, "btn-danger");
    g_signal_connect(btn_clear_all, "clicked", G_CALLBACK(on_btn_clear_all_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(dsa_vbox), btn_clear_all, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(dsa_frame), dsa_vbox);
    gtk_box_pack_start(GTK_BOX(left_panel), dsa_frame, FALSE, FALSE, 0);

    /* --- Right Panel --- */
    GtkWidget *right_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(content_hbox), right_panel, TRUE, TRUE, 0);

    /* C. Appliance Table Frame */
    GtkWidget *table_frame = gtk_frame_new("Appliance Registry");
    GtkWidget *table_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(table_vbox), 6);

    /* TreeView & ListStore */
    g_app.list_store = gtk_list_store_new(
        NUM_COLS,
        G_TYPE_INT,     /* COL_ID */
        G_TYPE_STRING,  /* COL_NAME */
        G_TYPE_INT,     /* COL_QTY */
        G_TYPE_STRING,  /* COL_POWER_STR */
        G_TYPE_STRING,  /* COL_HOURS_STR */
        G_TYPE_STRING,  /* COL_DAILY_STR */
        G_TYPE_STRING,  /* COL_MONTHLY_STR */
        G_TYPE_STRING   /* COL_COST_STR */
    );

    g_app.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(g_app.list_store));
    g_object_unref(g_app.list_store); /* View now owns a reference to model */
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_app.tree_view), TRUE);
    gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_GRID_LINES_BOTH);

    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("ID", COL_ID)));
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("Appliance Name", COL_NAME)));
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("Qty", COL_QTY)));
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("Power (W)", COL_POWER_STR)));
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("Hours/Day", COL_HOURS_STR)));
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("Daily (kWh)", COL_DAILY_STR)));
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("Monthly (kWh)", COL_MONTHLY_STR)));
    gtk_tree_view_append_column(GTK_TREE_VIEW(g_app.tree_view), GTK_TREE_VIEW_COLUMN(create_table_column("Est. Cost", COL_COST_STR)));

    g_signal_connect(g_app.tree_view, "row-activated", G_CALLBACK(on_tree_view_row_activated), NULL);

    GtkWidget *scroll_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll_window), g_app.tree_view);
    gtk_box_pack_start(GTK_BOX(table_vbox), scroll_window, TRUE, TRUE, 0);

    /* Table action buttons */
    GtkWidget *table_actions_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *btn_edit = gtk_button_new_with_label("Edit Selected");
    g_signal_connect(btn_edit, "clicked", G_CALLBACK(on_btn_edit_clicked), NULL);

    GtkWidget *btn_delete = gtk_button_new_with_label("Delete Selected");
    GtkStyleContext *del_ctx = gtk_widget_get_style_context(btn_delete);
    gtk_style_context_add_class(del_ctx, "btn-danger");
    g_signal_connect(btn_delete, "clicked", G_CALLBACK(on_btn_delete_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(table_actions_box), btn_edit, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(table_actions_box), btn_delete, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(table_vbox), table_actions_box, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(table_frame), table_vbox);
    gtk_box_pack_start(GTK_BOX(right_panel), table_frame, TRUE, TRUE, 0);

    /* D. Summary Card Frame */
    GtkWidget *summary_frame = gtk_frame_new("Energy Consumption & Cost Summary");
    GtkWidget *summary_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(summary_vbox), 8);

    GtkWidget *summary_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(summary_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(summary_grid), 16);

    /* Row 0: Appliances Count & Units */
    GtkWidget *lbl_t1 = gtk_label_new("Total Registered:");
    gtk_widget_set_halign(lbl_t1, GTK_ALIGN_END);
    g_app.lbl_sum_appliances = gtk_label_new("0 appliances (0 total units)");
    gtk_widget_set_halign(g_app.lbl_sum_appliances, GTK_ALIGN_START);
    GtkStyleContext *val1_ctx = gtk_widget_get_style_context(g_app.lbl_sum_appliances);
    gtk_style_context_add_class(val1_ctx, "summary-val");
    gtk_grid_attach(GTK_GRID(summary_grid), lbl_t1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(summary_grid), g_app.lbl_sum_appliances, 1, 0, 1, 1);

    /* Row 1: Daily Energy */
    GtkWidget *lbl_t2 = gtk_label_new("Daily Consumption:");
    gtk_widget_set_halign(lbl_t2, GTK_ALIGN_END);
    g_app.lbl_sum_daily = gtk_label_new("0.000 kWh / day");
    gtk_widget_set_halign(g_app.lbl_sum_daily, GTK_ALIGN_START);
    GtkStyleContext *val2_ctx = gtk_widget_get_style_context(g_app.lbl_sum_daily);
    gtk_style_context_add_class(val2_ctx, "summary-val");
    gtk_grid_attach(GTK_GRID(summary_grid), lbl_t2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(summary_grid), g_app.lbl_sum_daily, 1, 1, 1, 1);

    /* Row 2: Monthly Energy */
    GtkWidget *lbl_t3 = gtk_label_new("Monthly Consumption:");
    gtk_widget_set_halign(lbl_t3, GTK_ALIGN_END);
    g_app.lbl_sum_monthly = gtk_label_new("0.00 kWh / month");
    gtk_widget_set_halign(g_app.lbl_sum_monthly, GTK_ALIGN_START);
    GtkStyleContext *val3_ctx = gtk_widget_get_style_context(g_app.lbl_sum_monthly);
    gtk_style_context_add_class(val3_ctx, "summary-highlight");
    gtk_grid_attach(GTK_GRID(summary_grid), lbl_t3, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(summary_grid), g_app.lbl_sum_monthly, 1, 2, 1, 1);

    /* Row 3: Estimated Bill */
    GtkWidget *lbl_t4 = gtk_label_new("Estimated Monthly Bill:");
    gtk_widget_set_halign(lbl_t4, GTK_ALIGN_END);
    g_app.lbl_sum_bill = gtk_label_new("INR 0.00");
    gtk_widget_set_halign(g_app.lbl_sum_bill, GTK_ALIGN_START);
    GtkStyleContext *val4_ctx = gtk_widget_get_style_context(g_app.lbl_sum_bill);
    gtk_style_context_add_class(val4_ctx, "summary-highlight");
    gtk_grid_attach(GTK_GRID(summary_grid), lbl_t4, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(summary_grid), g_app.lbl_sum_bill, 1, 3, 1, 1);

    /* Row 4: Top Consumer */
    GtkWidget *lbl_t5 = gtk_label_new("Highest Consuming Appliance:");
    gtk_widget_set_halign(lbl_t5, GTK_ALIGN_END);
    g_app.lbl_sum_highest = gtk_label_new("None");
    gtk_widget_set_halign(g_app.lbl_sum_highest, GTK_ALIGN_START);
    GtkStyleContext *val5_ctx = gtk_widget_get_style_context(g_app.lbl_sum_highest);
    gtk_style_context_add_class(val5_ctx, "summary-top");
    gtk_grid_attach(GTK_GRID(summary_grid), lbl_t5, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(summary_grid), g_app.lbl_sum_highest, 1, 4, 1, 1);

    gtk_box_pack_start(GTK_BOX(summary_vbox), summary_grid, FALSE, FALSE, 0);

    /* PBL Disclaimer Label */
    GtkWidget *lbl_pbl = gtk_label_new(
        "* Note: This tool provides estimations based on nameplate wattage. Actual billing varies by duty cycle, inverter tech, and progressive slab tariffs."
    );
    gtk_widget_set_halign(lbl_pbl, GTK_ALIGN_START);
    GtkStyleContext *pbl_ctx = gtk_widget_get_style_context(lbl_pbl);
    gtk_style_context_add_class(pbl_ctx, "pbl-notice");
    gtk_box_pack_start(GTK_BOX(summary_vbox), lbl_pbl, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(summary_frame), summary_vbox);
    gtk_box_pack_start(GTK_BOX(right_panel), summary_frame, FALSE, FALSE, 0);
}

/*
 * ============================================================================
 * MAIN ENTRY POINT
 * ============================================================================
 */

int main(int argc, char *argv[]) {
    /* Initialize GTK */
    gtk_init(&argc, &argv);

    /* Initialize internal data structures */
    memset(&g_app, 0, sizeof(AppWidgets));
    g_app.list = create_appliance_list();
    g_app.current_rate = DEFAULT_RATE;
    g_app.current_days = DEFAULT_DAYS;
    g_app.editing_id = 0;

    /* Build Desktop GUI */
    build_gui();
    apply_custom_css();

    /* Load initial appliances from data.txt if present */
    double file_rate = g_app.current_rate;
    int file_days = g_app.current_days;
    if (load_appliances_from_file(g_app.list, DEFAULT_DATA_FILE, &file_rate, &file_days)) {
        g_app.current_rate = file_rate;
        g_app.current_days = file_days;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_rate), file_rate);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(g_app.spin_days), (double)file_days);
    }

    refresh_gui_state();

    gtk_widget_show_all(g_app.main_window);

    /* Run GTK main event loop */
    gtk_main();

    /* Clean up allocated dynamic memory on application exit */
    free_appliance_list(g_app.list);

    return 0;
}
