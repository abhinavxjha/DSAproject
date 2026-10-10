#ifndef ENERGY_H
#define ENERGY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_NAME_LEN 64
#define DEFAULT_RATE 8.00
#define DEFAULT_DAYS 30
#define DEFAULT_DATA_FILE "data.txt"

/*
 * ============================================================================
 * DATA STRUCTURE DESIGN
 * ============================================================================
 * Appliance: Singly linked list node representing an electrical appliance.
 * Singly linked list is selected for dynamic insertion, deletion, and in-place
 * O(N log N) Merge Sort without reallocating arrays.
 */
typedef struct Appliance {
    int id;                      /* Unique appliance identifier (monotonically increasing) */
    char name[MAX_NAME_LEN];     /* Appliance label (e.g. "Air Conditioner", "Ceiling Fan") */
    int quantity;                /* Number of identical units (> 0) */
    double power_watts;          /* Rated power consumption per unit in Watts (> 0) */
    double hours_per_day;        /* Average daily operation hours (0.0 < hours <= 24.0) */
    struct Appliance *next;      /* Pointer to the next appliance node in the linked list */
} Appliance;

/*
 * ApplianceList: Container structure managing the linked list head and metadata.
 */
typedef struct ApplianceList {
    Appliance *head;             /* Pointer to the first appliance node */
    int count;                   /* Total number of distinct appliances in the list */
    int next_id;                 /* Generator for unique sequential appliance IDs */
} ApplianceList;

/*
 * EnergySummary: Aggregate metrics calculated dynamically across all appliances.
 */
typedef struct EnergySummary {
    int total_appliances;        /* Count of distinct appliance entries */
    int total_units;             /* Aggregate unit count across all appliances */
    double total_daily_kwh;      /* Sum of daily energy consumption (kWh/day) */
    double total_monthly_kwh;    /* Sum of monthly energy consumption (kWh/month) */
    double estimated_bill;       /* Estimated electricity bill for the period in Rupees */
    char highest_consumer_name[MAX_NAME_LEN]; /* Name of the highest consuming appliance */
    double highest_daily_kwh;    /* Daily energy consumption of the top consumer */
    double highest_monthly_kwh;  /* Monthly energy consumption of the top consumer */
    double highest_cost;         /* Estimated monthly cost of the top consumer */
    double highest_percentage;   /* Share of the highest consumer in total energy (%) */
} EnergySummary;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/* --- Memory Management & Lifecycle --- */
ApplianceList* create_appliance_list(void);
void free_appliance_list(ApplianceList *list);
void clear_all_appliances(ApplianceList *list);
Appliance* create_appliance_node(int id, const char *name, int quantity, double power_watts, double hours_per_day);

/* --- Linked List Operations (CRUD) --- */
bool add_appliance(ApplianceList *list, const char *name, int quantity, double power_watts, double hours_per_day);
bool update_appliance(ApplianceList *list, int id, const char *name, int quantity, double power_watts, double hours_per_day);
bool delete_appliance(ApplianceList *list, int id);

/* --- Energy & Cost Calculations --- */
double calculate_daily_kwh(const Appliance *app);
double calculate_monthly_kwh(const Appliance *app, int days);
double calculate_appliance_cost(const Appliance *app, int days, double rate_per_kwh);
void calculate_summary(const ApplianceList *list, int days, double rate_per_kwh, EnergySummary *summary);

/* --- Searching Algorithms (DSA: Linear Search & Substring Match) --- */
Appliance* search_appliance_by_id(const ApplianceList *list, int id);
Appliance* search_appliance_by_name(const ApplianceList *list, const char *query);

/* --- Sorting Algorithm (DSA: Recursive Merge Sort on Linked List) --- */
void sort_appliances_by_consumption(ApplianceList *list, int days);

/* --- File Handling (Local Text Persistence) --- */
bool save_appliances_to_file(const ApplianceList *list, const char *filename, double rate, int days);
bool load_appliances_from_file(ApplianceList *list, const char *filename, double *rate, int *days);

/* --- Input & Settings Validation --- */
bool validate_appliance_input(const char *name, int quantity, double power_watts, double hours_per_day, char *err_msg, size_t err_msg_len);
bool validate_settings(double rate, int days, char *err_msg, size_t err_msg_len);

#endif /* ENERGY_H */
