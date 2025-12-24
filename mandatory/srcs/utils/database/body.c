/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   body.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 23:50:39 by marvin            #+#    #+#             */
/*   Updated: 2025/12/21 23:50:39 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Configuration constants
#define HEADERS_MAX 10
#define COL_NAME_MAX_LEN 30
#define ROWS_MAX 100
#define CELL_DATA_MAX_LEN 100
#define DATABASE_TITLE_MAX_LEN 50
#define DATABASE_FOOTER_MAX_LEN 100
#define COLS_MAX 10
#define INITIAL_CAPACITY 10
#define GROWTH_FACTOR 2

// Unicode box drawing characters
#define SYMBOL_UNICODE_BAR "┃"
#define SYMBOL_UNICODE_HEADER_TOP_LEFT "┏"
#define SYMBOL_UNICODE_HEADER_TOP_RIGHT "┓"
#define SYMBOL_UNICODE_HEADER_BOTTOM_LEFT "┗"
#define SYMBOL_UNICODE_HEADER_BOTTOM_RIGHT "┛"
#define SYMBOL_UNICODE_HEADER_T_JOINT "┳"
#define SYMBOL_UNICODE_FOOTER_T_JOINT "┻"
#define SYMBOL_UNICODE_HEADER_CROSS "╋"
#define SYMBOL_UNICODE_HEADER_HORIZONTAL_LINE "━"
#define SYMBOL_UNICODE_HEADER_VERTICAL_LINE "┃"

// ANSI color codes
#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN   "\033[36m"
#define COLOR_MAGENTA "\033[35m"

// Data type enum
typedef enum e_format {
    COL_STR,
    COL_INT,
    COL_FLOAT,
    COL_BOOL,
    COL_DATE
} t_format;

// Alignment enum
typedef enum e_align {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER
} t_align;

// Column definition
typedef struct s_col {
    char *name;
    t_format format;
    t_align align;
    size_t width;
} t_col;

// Row structure
typedef struct s_row {
    char **data;
    size_t ncols;
} t_row;

typedef struct s_color
{
	uint8_t	r;
	uint8_t	g;
	uint8_t	b;
}	t_color;

typedef struct s_format_style
{
	t_align number_type;
	t_align string_type;
	t_color	title;
	t_color	header;
	t_color	footer;
	t_color even_row;
	t_color odd_row;
}t_format_style;
// Database structure
typedef struct s_database {
    char *title;
    t_col *cols;
    size_t ncols;
    size_t cols_capacity;
    t_row *rows;
    size_t nrows;
    size_t rows_capacity;
    char *footer;
} t_database;

// ============= UTILITY FUNCTIONS =============

static char *ft_strdup(const char *s) {
    size_t len;
    char *dup;
    
    if (!s) return NULL;
    len = strlen(s);
    dup = malloc(len + 1);
    if (!dup) return NULL;
    memcpy(dup, s, len + 1);
    return dup;
}

static size_t ft_strlen(const char *s) {
    return s ? strlen(s) : 0;
}

static size_t max_size(size_t a, size_t b) {
    return a > b ? a : b;
}

// ============= DATABASE CREATION =============

t_database *db_create(const char *title) {
    t_database *db;
    
    db = calloc(1, sizeof(t_database));
    if (!db) return NULL;
    
    db->title = title ? ft_strdup(title) : ft_strdup("Untitled Database");
    db->cols_capacity = INITIAL_CAPACITY;
    db->rows_capacity = INITIAL_CAPACITY;
    
    db->cols = calloc(db->cols_capacity, sizeof(t_col));
    db->rows = calloc(db->rows_capacity, sizeof(t_row));
    
    if (!db->cols || !db->rows) {
        free(db->title);
        free(db->cols);
        free(db->rows);
        free(db);
        return NULL;
    }
    
    return db;
}

// ============= COLUMN MANAGEMENT =============

int db_add_column(t_database *db, const char *name, t_format format, t_align align) {
    t_col *new_cols;
    size_t i;
    
    if (!db || !name) return -1;
    
    // Check if reallocation is needed
    if (db->ncols >= db->cols_capacity) {
        db->cols_capacity *= GROWTH_FACTOR;
        new_cols = realloc(db->cols, db->cols_capacity * sizeof(t_col));
        if (!new_cols) return -1;
        db->cols = new_cols;
    }
    
    // Initialize the new column
    db->cols[db->ncols].name = ft_strdup(name);
    db->cols[db->ncols].format = format;
    db->cols[db->ncols].align = align;
    db->cols[db->ncols].width = 0; // Dynamic width
    
    // Expand all existing rows to accommodate new column
    for (i = 0; i < db->nrows; i++) {
        char **new_data = realloc(db->rows[i].data, (db->ncols + 1) * sizeof(char *));
        if (!new_data) return -1;
        db->rows[i].data = new_data;
        db->rows[i].data[db->ncols] = ft_strdup("");
        db->rows[i].ncols = db->ncols + 1;
    }
    
    db->ncols++;
    return 0;
}

// ============= ROW MANAGEMENT =============

int db_add_row(t_database *db, ...) {
    va_list args;
    t_row *new_rows;
    size_t i;
    
    if (!db) return -1;
    
    // Check if reallocation is needed
    if (db->nrows >= db->rows_capacity) {
        db->rows_capacity *= GROWTH_FACTOR;
        new_rows = realloc(db->rows, db->rows_capacity * sizeof(t_row));
        if (!new_rows) return -1;
        db->rows = new_rows;
    }
    
    // Allocate data array for the new row
    db->rows[db->nrows].data = calloc(db->ncols, sizeof(char *));
    if (!db->rows[db->nrows].data) return -1;
    db->rows[db->nrows].ncols = db->ncols;
    
    // Fill in the data using varargs
    va_start(args, db);
    for (i = 0; i < db->ncols; i++) {
        const char *val = va_arg(args, const char *);
        db->rows[db->nrows].data[i] = val ? ft_strdup(val) : ft_strdup("");
    }
    va_end(args);
    
    db->nrows++;
    return 0;
}

int db_add_row_array(t_database *db, const char **values, size_t nvalues) {
    t_row *new_rows;
    size_t i;
    
    if (!db || !values) return -1;
    
    if (db->nrows >= db->rows_capacity) {
        db->rows_capacity *= GROWTH_FACTOR;
        new_rows = realloc(db->rows, db->rows_capacity * sizeof(t_row));
        if (!new_rows) return -1;
        db->rows = new_rows;
    }
    
    db->rows[db->nrows].data = calloc(db->ncols, sizeof(char *));
    if (!db->rows[db->nrows].data) return -1;
    db->rows[db->nrows].ncols = db->ncols;
    
    for (i = 0; i < db->ncols; i++) {
        if (i < nvalues && values[i]) {
            db->rows[db->nrows].data[i] = ft_strdup(values[i]);
        } else {
            db->rows[db->nrows].data[i] = ft_strdup("");
        }
    }
    
    db->nrows++;
    return 0;
}

// ============= COLUMN WIDTH CALCULATION =============

static void calculate_column_widths(t_database *db, size_t *widths) {
    size_t i, j, len;
    
    // Initialize with column name lengths
    for (i = 0; i < db->ncols; i++) {
        widths[i] = ft_strlen(db->cols[i].name);
    }
    
    // Find maximum length in each column
    for (i = 0; i < db->nrows; i++) {
        for (j = 0; j < db->ncols; j++) {
            len = ft_strlen(db->rows[i].data[j]);
            widths[j] = max_size(widths[j], len);
        }
    }
    
    // Apply fixed widths if specified
    for (i = 0; i < db->ncols; i++) {
        if (db->cols[i].width > 0) {
            widths[i] = db->cols[i].width;
        }
    }
}

// ============= CELL FORMATTING =============

static void format_cell(char *buf, const char *text, size_t width, t_align align) {
    size_t len = ft_strlen(text);
    size_t padding, left_pad, right_pad;
    
    if (len >= width) {
        snprintf(buf, width + 1, "%s", text);
        return;
    }
    
    padding = width - len;
    
    switch (align) {
        case ALIGN_LEFT:
            snprintf(buf, width + 1, "%-*s", (int)width, text);
            break;
        case ALIGN_RIGHT:
            snprintf(buf, width + 1, "%*s", (int)width, text);
            break;
        case ALIGN_CENTER:
            left_pad = padding / 2;
            right_pad = padding - left_pad;
            snprintf(buf, width + 1, "%*s%s%*s", (int)left_pad, "", text, (int)right_pad, "");
            break;
    }
}

// ============= TABLE RENDERING =============

static void print_separator(size_t *widths, size_t ncols, const char *left, 
                           const char *mid, const char *right, const char *line) {
    size_t i, j;
    
    printf("%s", left);
    for (i = 0; i < ncols; i++) {
        for (j = 0; j < widths[i] + 2; j++) {
            printf("%s", line);
        }
        if (i < ncols - 1) {
            printf("%s", mid);
        }
    }
    printf("%s\n", right);
}

void db_render(t_database *db) {
    size_t *widths;
    size_t i, j;
    char cell_buf[CELL_DATA_MAX_LEN];
    
    if (!db || db->ncols == 0) return;
    
    widths = calloc(db->ncols, sizeof(size_t));
    if (!widths) return;
    
    calculate_column_widths(db, widths);
    
    // Print title
    if (db->title) {
        printf("\n%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, db->title, COLOR_RESET);
    }
    
    // Top border
    print_separator(widths, db->ncols, SYMBOL_UNICODE_HEADER_TOP_LEFT,
                   SYMBOL_UNICODE_HEADER_T_JOINT, SYMBOL_UNICODE_HEADER_TOP_RIGHT,
                   SYMBOL_UNICODE_HEADER_HORIZONTAL_LINE);
    
    // Headers
    printf("%s", SYMBOL_UNICODE_BAR);
    for (i = 0; i < db->ncols; i++) {
        format_cell(cell_buf, db->cols[i].name, widths[i], ALIGN_CENTER);
        printf(" %s%s%s ", COLOR_BOLD, cell_buf, COLOR_RESET);
        printf("%s", SYMBOL_UNICODE_BAR);
    }
    printf("\n");
    
    // Header separator
    print_separator(widths, db->ncols, SYMBOL_UNICODE_HEADER_BOTTOM_LEFT,
                   SYMBOL_UNICODE_HEADER_CROSS, SYMBOL_UNICODE_HEADER_BOTTOM_RIGHT,
                   SYMBOL_UNICODE_HEADER_HORIZONTAL_LINE);
    
    // Data rows
    for (i = 0; i < db->nrows; i++) {
        printf("%s", SYMBOL_UNICODE_BAR);
        for (j = 0; j < db->ncols; j++) {
            t_align align = db->cols[j].align;
            // Auto-align numbers to right if not specified
            if (db->cols[j].format == COL_INT || db->cols[j].format == COL_FLOAT) {
                align = ALIGN_RIGHT;
            }
            format_cell(cell_buf, db->rows[i].data[j], widths[j], align);
            printf(" %s ", cell_buf);
            printf("%s", SYMBOL_UNICODE_BAR);
        }
        printf("\n");
    }
    
    // Bottom border
    print_separator(widths, db->ncols, SYMBOL_UNICODE_HEADER_BOTTOM_LEFT,
                   SYMBOL_UNICODE_FOOTER_T_JOINT, SYMBOL_UNICODE_HEADER_BOTTOM_RIGHT,
                   SYMBOL_UNICODE_HEADER_HORIZONTAL_LINE);
    
    if (db->footer) {
        printf("%s\n", db->footer);
    }
    
    free(widths);
}

// ============= CLEANUP =============

void db_free(t_database *db) {
    size_t i, j;
    
    if (!db) return;
    
    free(db->title);
    free(db->footer);
    
    for (i = 0; i < db->ncols; i++) {
        free(db->cols[i].name);
    }
    free(db->cols);
    
    for (i = 0; i < db->nrows; i++) {
        for (j = 0; j < db->rows[i].ncols; j++) {
            free(db->rows[i].data[j]);
        }
        free(db->rows[i].data);
    }
    free(db->rows);
    
    free(db);
}

// ============= EXAMPLE USAGE =============

int main(void) {
    t_database *db;
    
    // Create database
    db = db_create("Employee Database");
    if (!db) return 1;
    
    // Add columns
    db_add_column(db, "ID", COL_INT, ALIGN_RIGHT);
    db_add_column(db, "First Name", COL_STR, ALIGN_LEFT);
    db_add_column(db, "Last Name", COL_STR, ALIGN_LEFT);
    db_add_column(db, "Age", COL_INT, ALIGN_RIGHT);
    db_add_column(db, "Salary", COL_FLOAT, ALIGN_RIGHT);
    
    // Add rows
    db_add_row(db, "1", "John", "Doe", "30", "75000.50");
    db_add_row(db, "2", "Jane", "Smith", "25", "82000.00");
    db_add_row(db, "3", "Bob", "Johnson", "40", "95000.75");
    db_add_row(db, "4", "Alice", "Williams", "35", "88000.25");
    
    // Render table
    db_render(db);
    
    // Add another column dynamically
    printf("\n%sAdding a new column...%s\n", COLOR_YELLOW, COLOR_RESET);
    db_add_column(db, "Department", COL_STR, ALIGN_LEFT);
    
    // Update rows with department info (you'd need update functions)
    free(db->rows[0].data[5]);
    db->rows[0].data[5] = ft_strdup("Engineering");
    free(db->rows[1].data[5]);
    db->rows[1].data[5] = ft_strdup("Marketing");
    free(db->rows[2].data[5]);
    db->rows[2].data[5] = ft_strdup("Sales");
    free(db->rows[3].data[5]);
    db->rows[3].data[5] = ft_strdup("HR");
    
    db_render(db);
    
    // Cleanup
    db_free(db);
    
    return 0;
}