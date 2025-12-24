/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   generator.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/21 23:50:31 by marvin            #+#    #+#             */
/*   Updated: 2025/12/21 23:50:31 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>

// Configuration constants
#define INITIAL_CAPACITY 10
#define GROWTH_FACTOR 2
#define MAX_LINE_LENGTH 4096

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
#define SYMBOL_UNICODE_VERTICAL_T_LEFT "┣"
#define SYMBOL_UNICODE_VERTICAL_T_RIGHT "┫"

// ANSI color codes
#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"

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

// Color structure
typedef struct s_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} t_color;

// Format style configuration
typedef struct s_format_style {
    t_align number_align;
    t_align string_align;
    t_color title;
    t_color header;
    t_color footer;
    t_color even_row;
    t_color odd_row;
} t_format_style;

// Display configuration flags
typedef struct s_display_config {
    bool show_title;
    bool show_header;
    bool show_footer;
    bool show_row_numbers;
    bool show_left_attributes;  // Show row labels on the left
    bool alternating_colors;
    bool use_bold_header;
    char col_separator;
} t_display_config;

// Column definition
typedef struct s_col {
    char *name;
    t_format format;
    t_align align;
    size_t width;
} t_col;

// Row structure
typedef struct s_row {
    char *label;  // Optional row label/attribute
    char **data;
    size_t ncols;
} t_row;

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
    t_format_style style;
    t_display_config config;
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

static void free_split(char **arr);
static char *trim_whitespace(char *str);

/* ensure wcwidth is declared on all platforms to avoid implicit-declaration warnings */
extern int wcwidth(wchar_t);

/* forward declarations to avoid implicit-declaration/errors */
static int  display_width(const char *s);
static void utf8_truncate_by_display_width(const char *src, size_t max_display, char *dst, size_t dst_size);

// Proper CSV split function that handles quoted fields and escaped quotes ("")
static char **ft_split(const char *str, char delimiter) {
    char **result = NULL;
    size_t count = 0;
    const char *p = str;
    bool in_quotes = false;

    if (!str)
        return NULL;

    // First pass: count fields
    while (*p) {
        if (*p == '"') {
            // enter quoted section
            in_quotes = true;
            p++;
            while (*p) {
                if (*p == '"' && p[1] == '"') {
                    p += 2; // escaped quote
                    continue;
                }
                if (*p == '"') { // closing quote
                    p++;
                    in_quotes = false;
                    break;
                }
                p++;
            }
        } else if (*p == delimiter && !in_quotes) {
            count++;
            p++;
        } else {
            p++;
        }
    }
    count++; // last field

    result = calloc(count + 1, sizeof(char *));
    if (!result)
        return NULL;

    // Second pass: extract fields
    p = str;
    size_t idx = 0;
    while (idx < count) {
        bool was_quoted = false;
        size_t write_pos = 0;
        size_t buf_cap = strlen(p) + 1;
        char *field = calloc(buf_cap + 1, 1);
        if (!field) {
            free_split(result);
            return NULL;
        }

        if (*p == '"') {
            was_quoted = true;
            p++; // skip opening quote
            while (*p) {
                if (*p == '"' && p[1] == '"') {
                    field[write_pos++] = '"';
                    p += 2;
                    continue;
                }
                if (*p == '"') { // closing
                    p++;
                    break;
                }
                field[write_pos++] = *p++;
            }
            // skip spaces/tabs until delimiter or end
            while (*p && *p != delimiter && (*p == ' ' || *p == '\t'))
                p++;
            if (*p == delimiter)
                p++;
        } else {
            while (*p && *p != delimiter) {
                field[write_pos++] = *p++;
            }
            if (*p == delimiter)
                p++;
        }
        field[write_pos] = '\0';

        if (!was_quoted) {
            char *trimmed = trim_whitespace(field);
            if (trimmed != field) {
                char *dup = ft_strdup(trimmed);
                free(field);
                field = dup ? dup : ft_strdup("");
            }
        }

        result[idx++] = field;
    }
    result[idx] = NULL;
    return result;
}

static void free_split(char **arr) {
    size_t i;
    if (!arr) return;
    for (i = 0; arr[i]; i++) {
        free(arr[i]);
    }
    free(arr);
}

static char *trim_whitespace(char *str) {
    char *end;
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end + 1) = 0;
    return str;
}

// ============= COLOR FUNCTIONS =============

static void print_rgb_color(t_color c) {
    printf("\033[38;2;%d;%d;%dm", c.r, c.g, c.b);
}

static t_color make_color(uint8_t r, uint8_t g, uint8_t b) {
    t_color c = {r, g, b};
    return c;
}

// ============= DEFAULT CONFIGURATIONS =============

static t_format_style default_style(void) {
    t_format_style style;
    style.number_align = ALIGN_RIGHT;
    style.string_align = ALIGN_LEFT;
    style.title = make_color(0, 255, 255);      // Cyan
    style.header = make_color(255, 255, 255);   // White
    style.footer = make_color(200, 200, 200);   // Light gray
    style.even_row = make_color(220, 220, 255); // Light blue
    style.odd_row = make_color(255, 255, 255);  // White
    return style;
}

static t_display_config default_config(void) {
    t_display_config cfg;
    cfg.show_title = true;
    cfg.show_header = true;
    cfg.show_footer = true;
    cfg.show_row_numbers = false;
    cfg.show_left_attributes = false;
    cfg.alternating_colors = true;
    cfg.use_bold_header = true;
    cfg.col_separator = ',';
    return cfg;
}

// ============= DATABASE CREATION =============

t_database *db_create(const char *title) {
    t_database *db;
    
    db = calloc(1, sizeof(t_database));
    if (!db) return NULL;

    /* ensure locale is set for wcwidth/mbrtowc */
    setlocale(LC_CTYPE, "");

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
    
    db->style = default_style();
    db->config = default_config();
    
    return db;
}

// ============= COLUMN MANAGEMENT =============

int db_add_column(t_database *db, const char *name, t_format format, t_align align) {
    t_col *new_cols;
    size_t i;
    
    if (!db || !name) return -1;
    
    if (db->ncols >= db->cols_capacity) {
        db->cols_capacity *= GROWTH_FACTOR;
        new_cols = realloc(db->cols, db->cols_capacity * sizeof(t_col));
        if (!new_cols) return -1;
        db->cols = new_cols;
    }
    
    db->cols[db->ncols].name = ft_strdup(name);
    db->cols[db->ncols].format = format;
    db->cols[db->ncols].align = align;
    db->cols[db->ncols].width = 0;
    
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

int db_add_row_with_label(t_database *db, const char *label, const char **values, size_t nvalues) {
    t_row *new_rows;
    size_t i;
    
    if (!db) return -1;
    
    if (db->nrows >= db->rows_capacity) {
        db->rows_capacity *= GROWTH_FACTOR;
        new_rows = realloc(db->rows, db->rows_capacity * sizeof(t_row));
        if (!new_rows) return -1;
        db->rows = new_rows;
    }
    
    db->rows[db->nrows].label = label ? ft_strdup(label) : NULL;
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

int db_add_row(t_database *db, ...) {
    va_list args;
    t_row *new_rows;
    size_t i;
    
    if (!db) return -1;
    
    if (db->nrows >= db->rows_capacity) {
        db->rows_capacity *= GROWTH_FACTOR;
        new_rows = realloc(db->rows, db->rows_capacity * sizeof(t_row));
        if (!new_rows) return -1;
        db->rows = new_rows;
    }
    
    db->rows[db->nrows].label = NULL;
    db->rows[db->nrows].data = calloc(db->ncols, sizeof(char *));
    if (!db->rows[db->nrows].data) return -1;
    db->rows[db->nrows].ncols = db->ncols;
    
    va_start(args, db);
    for (i = 0; i < db->ncols; i++) {
        const char *val = va_arg(args, const char *);
        db->rows[db->nrows].data[i] = val ? ft_strdup(val) : ft_strdup("");
    }
    va_end(args);
    
    db->nrows++;
    return 0;
}

// ============= FILE LOADING =============

int db_load_from_csv(t_database *db, const char *filename) {
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char **fields;
    size_t i, field_count;
    bool first_line = true;
    
    if (!db || !filename) return -1;
    
    fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return -1;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        
        fields = ft_split(line, db->config.col_separator);
        if (!fields) continue;
        
        // Count fields
        field_count = 0;
        while (fields[field_count]) field_count++;
        
        if (first_line) {
            // First line = headers
            for (i = 0; i < field_count; i++) {
                char *trimmed = trim_whitespace(fields[i]);
                db_add_column(db, trimmed, COL_STR, ALIGN_LEFT);
            }
            first_line = false;
        } else {
            // Data rows
            const char **data = (const char **)fields;
            db_add_row_with_label(db, NULL, data, field_count);
        }
        
        free_split(fields);
    }
    
    fclose(fp);
    printf("Loaded %zu rows from '%s'\n", db->nrows, filename);
    return 0;
}

// ============= COLUMN WIDTH CALCULATION =============

static void calculate_column_widths(t_database *db, size_t *widths, size_t *label_width) {
    size_t i, j;
    int len;

    *label_width = 0;

    // Calculate label width if needed
    if (db->config.show_left_attributes) {
        for (i = 0; i < db->nrows; i++) {
            if (db->rows[i].label) {
                len = display_width(db->rows[i].label);
                *label_width = max_size(*label_width, (size_t)len);
            }
        }
        *label_width = max_size(*label_width, 8); // Minimum width
    }

    // Initialize with column name display lengths (headers)
    for (i = 0; i < db->ncols; i++) {
        widths[i] = display_width(db->cols[i].name);
    }

    // Find maximum display width in each column
    for (i = 0; i < db->nrows; i++) {
        for (j = 0; j < db->ncols && j < db->rows[i].ncols; j++) {
            if (db->rows[i].data[j]) {
                len = display_width(db->rows[i].data[j]);
                widths[j] = max_size(widths[j], (size_t)len);
            }
        }
    }

    // Apply fixed widths if specified (override calculated widths)
    for (i = 0; i < db->ncols; i++) {
        if (db->cols[i].width > 0) {
            widths[i] = db->cols[i].width;
        }
    }
}

// return displayed column width (wcwidth-aware) of a UTF-8 string
static int display_width(const char *s)
{
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    const unsigned char *p = (const unsigned char *)s;
    size_t bytes;
    wchar_t wc;
    int w = 0;
    while (*p)
    {
        bytes = mbrtowc(&wc, (const char *)p, MB_CUR_MAX, &st);
        if (bytes == (size_t)-1 || bytes == (size_t)-2 || bytes == 0)
        {
            // invalid multibyte -> count as single column and advance one byte
            w += 1;
            p++;
            memset(&st, 0, sizeof(st));
            continue;
        }
        int cw = wcwidth(wc);
        if (cw < 0) cw = 0;
        w += cw;
        p += bytes;
    }
    return w;
}

/* copy from src to dst up to max_display_width display columns, preserving UTF-8 char boundaries */
static void utf8_truncate_by_display_width(const char *src, size_t max_display, char *dst, size_t dst_size)
{
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    const unsigned char *p = (const unsigned char *)src;
    size_t bytes;
    wchar_t wc;
    int acc = 0;
    size_t out = 0;

    if (!src || !dst || dst_size == 0) return;
    dst[0] = '\0';

    while (*p)
    {
        bytes = mbrtowc(&wc, (const char *)p, MB_CUR_MAX, &st);
        if (bytes == (size_t)-1 || bytes == (size_t)-2 || bytes == 0)
        {
            // invalid byte -> treat single byte as width 1
            if (acc + 1 > (int)max_display) break;
            if (out + 1 < dst_size) dst[out++] = *p;
            acc += 1;
            p++;
            memset(&st, 0, sizeof(st));
            continue;
        }
        int cw = wcwidth(wc);
        if (cw < 0) cw = 0;
        if (acc + cw > (int)max_display) break;
        if (out + bytes < dst_size)
        {
            memcpy(dst + out, p, bytes);
            out += bytes;
        }
        acc += cw;
        p += bytes;
    }
    if (out < dst_size) dst[out] = '\0';
    else dst[dst_size - 1] = '\0';
}

// ============= CELL FORMATTING =============

static void format_cell(char *buf, const char *text, size_t width, t_align align, size_t bufsize) {
    int dlen = display_width(text ? text : "");
    char tmp[MAX_LINE_LENGTH];

    if (!text) text = "";

    // Truncate if too long (by display width), producing valid UTF-8 in tmp
    if ((size_t)dlen > width) {
        utf8_truncate_by_display_width(text, width, tmp, sizeof(tmp));
        // copy truncated text into buf
        snprintf(buf, bufsize, "%s", tmp);
        return;
    }

    // Not too long — compute padding
    size_t pad = width - dlen;
    if (align == ALIGN_LEFT) {
        // text + pad spaces
        snprintf(buf, bufsize, "%s%*s", text, (int)pad, "");
    } else if (align == ALIGN_RIGHT) {
        snprintf(buf, bufsize, "%*s%s", (int)pad, "", text);
    } else { // center
        size_t left = pad / 2;
        size_t right = pad - left;
        // left spaces + text + right spaces
        snprintf(buf, bufsize, "%*s%s%*s", (int)left, "", text, (int)right, "");
    }
}

// ============= TABLE RENDERING =============

static void print_separator(size_t *widths, size_t ncols, size_t label_width,
                           bool show_label, const char *left, const char *mid, 
                           const char *right, const char *line) {
    size_t i, j;
    
    printf("%s", left);
    
    if (show_label) {
        for (j = 0; j < label_width + 2; j++) {
            printf("%s", line);
        }
        printf("%s", mid);
    }
    
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
    size_t label_width = 0;
    size_t i, j;
    char cell_buf[MAX_LINE_LENGTH];
    t_align align;
    
    if (!db || db->ncols == 0) return;
    
    widths = calloc(db->ncols, sizeof(size_t));
    if (!widths) return;
    
    calculate_column_widths(db, widths, &label_width);
    
    // Print title
    if (db->config.show_title && db->title) {
        printf("\n");
        if (db->config.use_bold_header) printf(COLOR_BOLD);
        print_rgb_color(db->style.title);
        printf("%s", db->title);
        printf(COLOR_RESET);
        printf("\n");
    }
    
    // Top border
    print_separator(widths, db->ncols, label_width, db->config.show_left_attributes,
                   SYMBOL_UNICODE_HEADER_TOP_LEFT, SYMBOL_UNICODE_HEADER_T_JOINT,
                   SYMBOL_UNICODE_HEADER_TOP_RIGHT, SYMBOL_UNICODE_HEADER_HORIZONTAL_LINE);
    
    // Headers
    if (db->config.show_header) {
        printf("%s", SYMBOL_UNICODE_BAR);
        
        if (db->config.show_left_attributes) {
            format_cell(cell_buf, "Attribute", label_width, ALIGN_CENTER, sizeof(cell_buf));
            if (db->config.use_bold_header) printf(COLOR_BOLD);
            print_rgb_color(db->style.header);
            printf(" %s ", cell_buf);
            printf(COLOR_RESET);
            printf("%s", SYMBOL_UNICODE_BAR);
        }
        
        for (i = 0; i < db->ncols; i++) {
            format_cell(cell_buf, db->cols[i].name, widths[i], ALIGN_CENTER, sizeof(cell_buf));
            if (db->config.use_bold_header) printf(COLOR_BOLD);
            print_rgb_color(db->style.header);
            printf(" %s ", cell_buf);
            printf(COLOR_RESET);
            printf("%s", SYMBOL_UNICODE_BAR);
        }
        printf("\n");
        
        // Header separator
        print_separator(widths, db->ncols, label_width, db->config.show_left_attributes,
                       SYMBOL_UNICODE_VERTICAL_T_LEFT, SYMBOL_UNICODE_HEADER_CROSS,
                       SYMBOL_UNICODE_VERTICAL_T_RIGHT, SYMBOL_UNICODE_HEADER_HORIZONTAL_LINE);
    }
    
    // Data rows
    for (i = 0; i < db->nrows; i++) {
        printf("%s", SYMBOL_UNICODE_BAR);
        
        // Row label/attribute
        if (db->config.show_left_attributes) {
            const char *label = db->rows[i].label ? db->rows[i].label : "";
            format_cell(cell_buf, label, label_width, ALIGN_LEFT, sizeof(cell_buf));
            
            if (db->config.alternating_colors) {
                print_rgb_color(i % 2 == 0 ? db->style.even_row : db->style.odd_row);
            }
            
            printf(" %s ", cell_buf);
            if (db->config.alternating_colors) printf(COLOR_RESET);
            printf("%s", SYMBOL_UNICODE_BAR);
        }
        
        // Data cells - ensure we respect column boundaries
        for (j = 0; j < db->ncols; j++) {
            // Determine alignment
            if (db->cols[j].align != ALIGN_LEFT) {
                align = db->cols[j].align;
            } else if (db->cols[j].format == COL_INT || db->cols[j].format == COL_FLOAT) {
                align = db->style.number_align;
            } else {
                align = db->style.string_align;
            }
            
            // Get cell data - handle missing columns gracefully
            const char *cell_data = "";
            if (j < db->rows[i].ncols && db->rows[i].data[j]) {
                cell_data = db->rows[i].data[j];
            }
            
            format_cell(cell_buf, cell_data, widths[j], align, sizeof(cell_buf));
            
            if (db->config.alternating_colors) {
                print_rgb_color(i % 2 == 0 ? db->style.even_row : db->style.odd_row);
            }
            
            printf(" %s ", cell_buf);
            if (db->config.alternating_colors) printf(COLOR_RESET);
            printf("%s", SYMBOL_UNICODE_BAR);
        }
        printf("\n");
    }
    
    // Bottom border
    print_separator(widths, db->ncols, label_width, db->config.show_left_attributes,
                   SYMBOL_UNICODE_HEADER_BOTTOM_LEFT, SYMBOL_UNICODE_FOOTER_T_JOINT,
                   SYMBOL_UNICODE_HEADER_BOTTOM_RIGHT, SYMBOL_UNICODE_HEADER_HORIZONTAL_LINE);
    
    if (db->config.show_footer && db->footer) {
        print_rgb_color(db->style.footer);
        printf("%s", db->footer);
        printf(COLOR_RESET);
        printf("\n");
    }
    
    free(widths);
}

// ============= CONFIGURATION HELPERS =============

void db_set_style(t_database *db, t_format_style style) {
    if (db) db->style = style;
}

void db_set_config(t_database *db, t_display_config config) {
    if (db) db->config = config;
}

void db_set_footer(t_database *db, const char *footer) {
    if (!db) return;
    free(db->footer);
    db->footer = footer ? ft_strdup(footer) : NULL;
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
        free(db->rows[i].label);
        for (j = 0; j < db->rows[i].ncols; j++) {
            free(db->rows[i].data[j]);
        }
        free(db->rows[i].data);
    }
    free(db->rows);
    
    free(db);
}

// ============= EXAMPLE USAGE =============

void create_sample_database(t_database *db) {
    // Add columns
    db_add_column(db, "ID", COL_INT, ALIGN_RIGHT);
    db_add_column(db, "First Name", COL_STR, ALIGN_LEFT);
    db_add_column(db, "Last Name", COL_STR, ALIGN_LEFT);
    db_add_column(db, "Age", COL_INT, ALIGN_RIGHT);
    db_add_column(db, "Salary", COL_FLOAT, ALIGN_RIGHT);
    db_add_column(db, "Department", COL_STR, ALIGN_LEFT);
    
    // Add rows with labels
    const char *row1[] = {"1", "John", "Doe", "30", "75000.50", "Engineering"};
    db_add_row_with_label(db, "Employee 1", row1, 6);
    
    const char *row2[] = {"2", "Jane", "Smith", "25", "82000.00", "Marketing"};
    db_add_row_with_label(db, "Employee 2", row2, 6);
    
    const char *row3[] = {"3", "Bob", "Johnson", "40", "95000.75", "Sales"};
    db_add_row_with_label(db, "Employee 3", row3, 6);
    
    const char *row4[] = {"4", "Alice", "Williams", "35", "88000.25", "HR"};
    db_add_row_with_label(db, "Employee 4", row4, 6);
    
    const char *row5[] = {"5", "Charlie", "Brown", "28", "79000.00", "Engineering"};
    db_add_row_with_label(db, "Employee 5", row5, 6);
}

int main(int argc, char **argv) {
    t_database *db;
    t_display_config cfg;
    
    // Create database
    db = db_create("Employee Database");
    if (!db) return 1;
    
    // Check if file argument provided
    if (argc > 1) {
        printf("Loading data from file: %s\n", argv[1]);
        if (db_load_from_csv(db, argv[1]) == 0) {
            db->config.show_left_attributes = false;
            db_set_footer(db, "Data loaded from CSV file");
            db_render(db);
        }
    } else {
        // Generate sample data
        printf("No file provided, generating sample data...\n\n");
        
        create_sample_database(db);
        
        // Standard rendering
        db_set_footer(db, "Total Employees: 5");
        db_render(db);
        
        // Example with left attributes
        printf("\n\n=== WITH LEFT ATTRIBUTES ===\n");
        cfg = db->config;
        cfg.show_left_attributes = true;
        db_set_config(db, cfg);
        db_render(db);
        
        // Example with custom colors
        printf("\n\n=== WITH CUSTOM COLORS ===\n");
        t_format_style custom_style = db->style;
        custom_style.title = make_color(255, 100, 255);    // Magenta
        custom_style.header = make_color(100, 255, 100);   // Light green
        custom_style.even_row = make_color(255, 255, 200); // Light yellow
        custom_style.odd_row = make_color(200, 255, 255);  // Light cyan
        db_set_style(db, custom_style);
        db_render(db);
        
        // Example without alternating colors
        printf("\n\n=== WITHOUT ALTERNATING COLORS ===\n");
        cfg.alternating_colors = false;
        cfg.show_left_attributes = false;
        db_set_config(db, cfg);
        db_render(db);
    }
    
    // Cleanup
    db_free(db);
    
    return 0;
}