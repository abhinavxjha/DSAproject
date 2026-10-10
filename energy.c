#include "energy.h"
#include <ctype.h>

/* Helper for case-insensitive substring search across platforms */
static const char* strcasestr_custom(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    if (*needle == '\0') return haystack;

    for (; *haystack != '\0'; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            const char *h = haystack;
            const char *n = needle;
            while (*h != '\0' && *n != '\0' && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
                h++;
                n++;
            }
            if (*n == '\0') {
                return haystack;
            }
        }
    }
    return NULL;
}

/*
 * ============================================================================
 * MEMORY LIFECYCLE & NODE MANAGEMENT
 * ============================================================================
 */

ApplianceList* create_appliance_list(void) {
    ApplianceList *list = (ApplianceList*)malloc(sizeof(ApplianceList));
    if (!list) {
        perror("Failed to allocate memory for ApplianceList");
        return NULL;
    }
    list->head = NULL;
    list->count = 0;
    list->next_id = 1;
    return list;
}

void clear_all_appliances(ApplianceList *list) {
    if (!list) return;

    Appliance *curr = list->head;
    while (curr != NULL) {
        Appliance *temp = curr;
        curr = curr->next;
        free(temp);
    }
    list->head = NULL;
    list->count = 0;
    list->next_id = 1;
}

void free_appliance_list(ApplianceList *list) {
    if (!list) return;
    clear_all_appliances(list);
    free(list);
}

Appliance* create_appliance_node(int id, const char *name, int quantity, double power_watts, double hours_per_day) {
    Appliance *node = (Appliance*)malloc(sizeof(Appliance));
    if (!node) {
        perror("Failed to allocate memory for Appliance node");
        return NULL;
    }
    node->id = id;
    strncpy(node->name, name ? name : "Unnamed", MAX_NAME_LEN - 1);
    node->name[MAX_NAME_LEN - 1] = '\0';
    node->quantity = quantity;
    node->power_watts = power_watts;
    node->hours_per_day = hours_per_day;
    node->next = NULL;
    return node;
}

/*
 * ============================================================================
 * LINKED LIST OPERATIONS (CRUD)
 * ============================================================================
 */

bool add_appliance(ApplianceList *list, const char *name, int quantity, double power_watts, double hours_per_day) {
    if (!list) return false;

    Appliance *new_node = create_appliance_node(list->next_id, name, quantity, power_watts, hours_per_day);
    if (!new_node) return false;

    list->next_id++;
    list->count++;

    /* Append to tail to preserve natural insertion order */
    if (list->head == NULL) {
        list->head = new_node;
    } else {
        Appliance *curr = list->head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_node;
    }
    return true;
}

bool update_appliance(ApplianceList *list, int id, const char *name, int quantity, double power_watts, double hours_per_day) {
    if (!list) return false;

    Appliance *target = search_appliance_by_id(list, id);
    if (!target) return false;

    strncpy(target->name, name ? name : target->name, MAX_NAME_LEN - 1);
    target->name[MAX_NAME_LEN - 1] = '\0';
    target->quantity = quantity;
    target->power_watts = power_watts;
    target->hours_per_day = hours_per_day;
    return true;
}

bool delete_appliance(ApplianceList *list, int id) {
    if (!list || list->head == NULL) return false;

    Appliance *curr = list->head;
    Appliance *prev = NULL;

    while (curr != NULL && curr->id != id) {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) {
        return false; /* Not found */
    }

    if (prev == NULL) {
        /* Deleting head node */
        list->head = curr->next;
    } else {
        prev->next = curr->next;
    }

    free(curr);
    list->count--;
    return true;
}

/*
 * ============================================================================
 * CALCULATIONS
 * ============================================================================
 * Power (kW) = Power (W) / 1000.0
 * Daily Energy (kWh) = Power(kW) * Quantity * HoursPerDay
 * Monthly Energy (kWh) = Daily Energy * Days
 * Estimated Bill = Monthly Energy * RatePerKWh
 */

double calculate_daily_kwh(const Appliance *app) {
    if (!app) return 0.0;
    double power_kw = app->power_watts / 1000.0;
    return power_kw * (double)app->quantity * app->hours_per_day;
}

double calculate_monthly_kwh(const Appliance *app, int days) {
    if (!app || days <= 0) return 0.0;
    return calculate_daily_kwh(app) * (double)days;
}

double calculate_appliance_cost(const Appliance *app, int days, double rate_per_kwh) {
    if (!app || days <= 0 || rate_per_kwh <= 0.0) return 0.0;
    return calculate_monthly_kwh(app, days) * rate_per_kwh;
}

void calculate_summary(const ApplianceList *list, int days, double rate_per_kwh, EnergySummary *summary) {
    if (!summary) return;

    memset(summary, 0, sizeof(EnergySummary));
    strncpy(summary->highest_consumer_name, "None", MAX_NAME_LEN - 1);

    if (!list || list->head == NULL || days <= 0) {
        return;
    }

    summary->total_appliances = list->count;

    Appliance *curr = list->head;
    Appliance *top_app = NULL;
    double max_daily = -1.0;

    while (curr != NULL) {
        double daily = calculate_daily_kwh(curr);
        summary->total_units += curr->quantity;
        summary->total_daily_kwh += daily;

        if (daily > max_daily) {
            max_daily = daily;
            top_app = curr;
        }
        curr = curr->next;
    }

    summary->total_monthly_kwh = summary->total_daily_kwh * (double)days;
    summary->estimated_bill = summary->total_monthly_kwh * rate_per_kwh;

    if (top_app != NULL && max_daily >= 0.0) {
        snprintf(summary->highest_consumer_name, sizeof(summary->highest_consumer_name), "%s", top_app->name);
        summary->highest_daily_kwh = max_daily;
        summary->highest_monthly_kwh = max_daily * (double)days;
        summary->highest_cost = summary->highest_monthly_kwh * rate_per_kwh;

        if (summary->total_daily_kwh > 0.0) {
            summary->highest_percentage = (max_daily / summary->total_daily_kwh) * 100.0;
        } else {
            summary->highest_percentage = 0.0;
        }
    }
}

/*
 * ============================================================================
 * SEARCHING ALGORITHMS (DSA: Manual Traversal)
 * ============================================================================
 */

Appliance* search_appliance_by_id(const ApplianceList *list, int id) {
    if (!list) return NULL;
    Appliance *curr = list->head;
    while (curr != NULL) {
        if (curr->id == id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

Appliance* search_appliance_by_name(const ApplianceList *list, const char *query) {
    if (!list || !query || strlen(query) == 0) return NULL;
    Appliance *curr = list->head;
    while (curr != NULL) {
        if (strcasestr_custom(curr->name, query) != NULL) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

/*
 * ============================================================================
 * SORTING ALGORITHM (DSA: Recursive Merge Sort on Singly Linked List)
 * Time Complexity: O(N log N) in all cases.
 * Space Complexity: O(log N) recursive call stack; 0 heap reallocation.
 * Sort Order: Descending by monthly energy consumption (highest first).
 * ============================================================================
 */

/* Fast/slow pointer strategy to bisect the linked list */
static Appliance* split_list(Appliance *source) {
    Appliance *fast = source;
    Appliance *slow = source;

    while (fast->next != NULL && fast->next->next != NULL) {
        fast = fast->next->next;
        slow = slow->next;
    }

    Appliance *middle = slow->next;
    slow->next = NULL;
    return middle;
}

/* Merge two sorted sublists descending by monthly energy consumption */
static Appliance* merge_sorted_lists(Appliance *a, Appliance *b, int days) {
    if (!a) return b;
    if (!b) return a;

    double energy_a = calculate_monthly_kwh(a, days);
    double energy_b = calculate_monthly_kwh(b, days);

    Appliance *result = NULL;
    /* Descending order: larger energy consumption placed first */
    if (energy_a >= energy_b) {
        result = a;
        result->next = merge_sorted_lists(a->next, b, days);
    } else {
        result = b;
        result->next = merge_sorted_lists(a, b->next, days);
    }
    return result;
}

/* Recursive Divide-and-Conquer driver */
static Appliance* merge_sort_rec(Appliance *head, int days) {
    if (head == NULL || head->next == NULL) {
        return head;
    }

    Appliance *second_half = split_list(head);

    Appliance *left = merge_sort_rec(head, days);
    Appliance *right = merge_sort_rec(second_half, days);

    return merge_sorted_lists(left, right, days);
}

void sort_appliances_by_consumption(ApplianceList *list, int days) {
    if (!list || list->head == NULL || list->head->next == NULL) {
        return;
    }
    list->head = merge_sort_rec(list->head, days);
}

/*
 * ============================================================================
 * FILE PERSISTENCE (Local Text Storage: data.txt)
 * ============================================================================
 */

bool save_appliances_to_file(const ApplianceList *list, const char *filename, double rate, int days) {
    const char *target = filename ? filename : DEFAULT_DATA_FILE;
    FILE *fp = fopen(target, "w");
    if (!fp) {
        perror("Failed to open file for saving");
        return false;
    }

    /* Configuration header */
    fprintf(fp, "# SMART ENERGY TRACKER PERSISTENCE DATA\n");
    fprintf(fp, "RATE=%.2f\n", rate);
    fprintf(fp, "DAYS=%d\n", days);
    fprintf(fp, "# ID|NAME|QUANTITY|POWER_WATTS|HOURS_PER_DAY\n");

    if (list != NULL) {
        Appliance *curr = list->head;
        while (curr != NULL) {
            fprintf(fp, "%d|%s|%d|%.2f|%.2f\n",
                    curr->id,
                    curr->name,
                    curr->quantity,
                    curr->power_watts,
                    curr->hours_per_day);
            curr = curr->next;
        }
    }

    fclose(fp);
    return true;
}

bool load_appliances_from_file(ApplianceList *list, const char *filename, double *rate, int *days) {
    if (!list) return false;

    const char *target = filename ? filename : DEFAULT_DATA_FILE;
    FILE *fp = fopen(target, "r");
    if (!fp) {
        /* File doesn't exist yet; not an error on first run */
        return false;
    }

    clear_all_appliances(list);

    char line[256];
    int max_id = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Trim trailing newline and whitespace */
        char *end = line + strlen(line) - 1;
        while (end >= line && (*end == '\r' || *end == '\n' || isspace((unsigned char)*end))) {
            *end = '\0';
            end--;
        }

        /* Skip blank lines and comments */
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        /* Parse settings */
        if (strncmp(line, "RATE=", 5) == 0) {
            if (rate) *rate = atof(line + 5);
            continue;
        }
        if (strncmp(line, "DAYS=", 5) == 0) {
            if (days) *days = atoi(line + 5);
            continue;
        }

        /* Parse appliance record: ID|NAME|QUANTITY|POWER_WATTS|HOURS_PER_DAY */
        int id = 0;
        char name[MAX_NAME_LEN] = {0};
        int quantity = 0;
        double power = 0.0;
        double hours = 0.0;

        char *token = strtok(line, "|");
        if (!token) continue;
        id = atoi(token);

        token = strtok(NULL, "|");
        if (!token) continue;
        snprintf(name, sizeof(name), "%s", token);

        token = strtok(NULL, "|");
        if (!token) continue;
        quantity = atoi(token);

        token = strtok(NULL, "|");
        if (!token) continue;
        power = atof(token);

        token = strtok(NULL, "|");
        if (!token) continue;
        hours = atof(token);

        /* Validate and insert node */
        if (quantity > 0 && power > 0.0 && hours > 0.0 && hours <= 24.0) {
            Appliance *node = create_appliance_node(id, name, quantity, power, hours);
            if (node) {
                list->count++;
                if (id > max_id) max_id = id;

                if (list->head == NULL) {
                    list->head = node;
                } else {
                    Appliance *cur = list->head;
                    while (cur->next != NULL) cur = cur->next;
                    cur->next = node;
                }
            }
        }
    }

    fclose(fp);
    list->next_id = max_id + 1;
    return true;
}

/*
 * ============================================================================
 * INPUT VALIDATION
 * ============================================================================
 */

bool validate_appliance_input(const char *name, int quantity, double power_watts, double hours_per_day, char *err_msg, size_t err_msg_len) {
    if (!name || strlen(name) == 0) {
        if (err_msg) snprintf(err_msg, err_msg_len, "Appliance name cannot be empty.");
        return false;
    }

    /* Check for all-whitespace name */
    bool all_spaces = true;
    for (size_t i = 0; i < strlen(name); i++) {
        if (!isspace((unsigned char)name[i])) {
            all_spaces = false;
            break;
        }
    }
    if (all_spaces) {
        if (err_msg) snprintf(err_msg, err_msg_len, "Appliance name cannot consist solely of whitespace.");
        return false;
    }

    if (quantity <= 0) {
        if (err_msg) snprintf(err_msg, err_msg_len, "Quantity must be greater than zero (positive integer).");
        return false;
    }

    if (power_watts <= 0.0) {
        if (err_msg) snprintf(err_msg, err_msg_len, "Power rating must be greater than zero Watts.");
        return false;
    }

    if (hours_per_day <= 0.0 || hours_per_day > 24.0) {
        if (err_msg) snprintf(err_msg, err_msg_len, "Hours per day must be between 0.1 and 24.0 hours.");
        return false;
    }

    return true;
}

bool validate_settings(double rate, int days, char *err_msg, size_t err_msg_len) {
    if (rate <= 0.0) {
        if (err_msg) snprintf(err_msg, err_msg_len, "Electricity rate per unit must be greater than zero.");
        return false;
    }

    if (days <= 0 || days > 365) {
        if (err_msg) snprintf(err_msg, err_msg_len, "Billing period must be between 1 and 365 days.");
        return false;
    }

    return true;
}
