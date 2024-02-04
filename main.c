#include <xcb/xcb.h>
#include <xcb/xfixes.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define APPEND(array, value, type)                                   \
    do {                                                             \
        array = realloc(array, (array##_length + 1) * sizeof(type)); \
        if (array == NULL) {                                         \
            fprintf(stderr, "Memory allocation failure!\n");         \
            exit(EXIT_FAILURE);                                      \
        }                                                            \
        array[array##_length] = value;                               \
        array##_length++;                                            \
    } while (0)

// We track the cursor state manually because hiding the cursor seems to stack
// e.g. if you hide the cursor twice with xcb_xfixes_hide_cursor you have to
//      call xcb_xfixes_show_cursor twice
uint8_t cursor_hidden = 0;

char **titles = NULL;
size_t titles_length = 0;
char **classes = NULL;
size_t classes_length = 0;
char **instances = NULL;
size_t instances_length = 0;
xcb_window_t *window_ids = NULL;
size_t window_ids_length = 0;

xcb_atom_t intern_atom(xcb_connection_t *conn, const char *atom) {
    xcb_atom_t result = XCB_NONE;
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, strlen(atom), atom);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
    if (reply) {
        result = reply->atom;
    }

    free(reply);
    return result;
}

xcb_window_t get_active_window(xcb_connection_t *conn, xcb_window_t window, xcb_atom_t atom) {
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, atom, XCB_ATOM_WINDOW, 0, 1);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
    xcb_window_t result = XCB_WINDOW_NONE;
    if (reply->length > 0) {
        result = *(xcb_window_t*)xcb_get_property_value(reply);
    }
    free(reply);
    return result;
}

char *get_title(xcb_connection_t *conn, xcb_window_t window) {
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, -1);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
    const char *source_title = (char*)xcb_get_property_value(reply);

    char *title = strdup(source_title);
    free(reply);
    return title;
}

char *get_class(xcb_connection_t *conn, xcb_window_t window) {
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 0, -1);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
    const char *source_class = (char*)xcb_get_property_value(reply);

    char *class = strdup(source_class);
    free(reply);
    return class;
}

char *get_instance(xcb_connection_t *conn, xcb_window_t window) {
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 0, -1);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
    const char *source_class = (char*)xcb_get_property_value(reply);

    const char *source_instance = &source_class[strlen(source_class) + 1];
    char *instance = strdup(source_instance);
    free(reply);
    return instance;
}

void hide_cursor(xcb_connection_t *conn, xcb_window_t root_window) {
    if (!cursor_hidden) {
        xcb_xfixes_hide_cursor(conn, root_window);
        xcb_flush(conn);
    }
    cursor_hidden = 1;
}

void show_cursor(xcb_connection_t *conn, xcb_window_t root_window) {
    if (cursor_hidden) {
        xcb_xfixes_show_cursor(conn, root_window);
        xcb_flush(conn);
    }
    cursor_hidden = 0;
}

void update_cursor(xcb_connection_t *conn, xcb_window_t root_window, xcb_window_t window) {
    if (window == XCB_WINDOW_NONE) {
        show_cursor(conn, root_window);
        return;
    }

    uint8_t success = 0;
    if (titles_length > 0) {
        char *title = get_title(conn, window);
        for (size_t i = 0; i < titles_length; i++) {
            const char* value = titles[i];
            if (strcmp(title, value) == 0) {
                hide_cursor(conn, root_window);
                success = 1;
                break;
            }
        }
        free(title);
        if (success) return;
    }

    if (classes_length > 0) {
        char *class = get_class(conn, window);
        for (size_t i = 0; i < classes_length; i++) {
            const char* value = classes[i];
            if (strcmp(class, value) == 0) {
                hide_cursor(conn, root_window);
                success = 1;
                break;
            }
        }
        free(class);
        if (success) return;
    }

    if (instances_length > 0) {
        char *instance = get_instance(conn, window);
        for (size_t i = 0; i < instances_length; i++) {
            const char* value = instances[i];
            if (strcmp(instance, value) == 0) {
                hide_cursor(conn, root_window);
                success = 1;
                break;
            }
        }
        free(instance);
        if (success) return;
    }

    if (window_ids_length > 0) {
        for (size_t i = 0; i < window_ids_length; i++) {
            const xcb_window_t value = window_ids[i];
            if (window == value) {
                hide_cursor(conn, root_window);
                return;
            }
        }
    }

    show_cursor(conn, root_window);
}

void print_help(int exit_code) {
    fprintf(stderr, "corvo: [CONDITIONS]\n");
    fprintf(stderr, "  -h             print this help and exit\n\n");

    fprintf(stderr, "  CONDITIONS:\n");
    fprintf(stderr, "    -t TITLE     hide the cursor on windows with TITLE as their window title (WM_NAME)\n");
    fprintf(stderr, "    -c CLASS     hide the cursor on windows with CLASS as their class value (first value in WM_CLASS)\n");
    fprintf(stderr, "    -i INSTANCE  hide the cursor on windows with INSTANCE as their class value (second value in WM_CLASS)\n");
    fprintf(stderr, "    -w WINDOW    hide the cursor on the window with the ID WINDOW\n");

    exit(exit_code);
}

int main(int argc, char *argv[]) {
    char opt = 0;
    while ((opt = getopt(argc, argv, "ht:c:i:w:")) != -1) {
        switch (opt) {
            case 'h': {
                print_help(EXIT_SUCCESS);
            } break;
            case 't': {
                APPEND(titles, optarg, char *);
            } break;
            case 'c': {
                APPEND(classes, optarg, char *);
            } break;
            case 'i': {
                APPEND(instances, optarg, char *);
            } break;
            case 'w': {
                xcb_window_t id = strtol(optarg, NULL, 0);
                if (id == XCB_WINDOW_NONE) {
                    fprintf(stderr, "Unable to parse window ID '%s'!\n", optarg);
                    return EXIT_FAILURE;
                }
                APPEND(window_ids, id, xcb_window_t);
            } break;
            case '?': {
                print_help(EXIT_FAILURE);
            } break;
        }
    }

    size_t total_length = titles_length + classes_length + instances_length + window_ids_length;
    if (total_length == 0) {
        fprintf(stderr, "Must pass in at least one condition!\n");
        print_help(EXIT_FAILURE);
    }

    xcb_connection_t *conn;

    if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL))) {
        return 0;
    }

    xcb_atom_t active_window = intern_atom(conn, "_NET_ACTIVE_WINDOW");

    if (active_window == XCB_NONE) {
        fprintf(stderr, "_NET_ACTIVE_WINDOW atom not found!");
        return EXIT_FAILURE;
    }

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    uint32_t values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
    xcb_change_window_attributes(conn, screen->root, XCB_CW_EVENT_MASK, values);
    xcb_flush(conn);

    xcb_xfixes_query_version_cookie_t xfixes_cookie = xcb_xfixes_query_version(conn, 4, 0);
    xcb_xfixes_query_version_reply_t *xfixes_reply = xcb_xfixes_query_version_reply(conn, xfixes_cookie, NULL);
    if (xfixes_reply->major_version < 4) {
        fprintf(stderr,
                "Xfixes version 4.0 or greater required, you have %d.%d\n",
                xfixes_reply->major_version, xfixes_reply->minor_version);
        return EXIT_FAILURE;
    }
    
    update_cursor(conn, screen->root, get_active_window(conn, screen->root, active_window));

    while (1) {
        xcb_generic_event_t *ev = xcb_wait_for_event(conn);
        if (!ev) {
            fprintf(stderr, "Error in xcb event loop!");
            return EXIT_FAILURE;
        }

        switch (ev->response_type & ~0x80) {
            case XCB_PROPERTY_NOTIFY: {
                xcb_property_notify_event_t *e = (xcb_property_notify_event_t *)ev;
                if (e->atom == active_window) {
                    xcb_window_t window = get_active_window(conn, screen->root, active_window);
                    update_cursor(conn, screen->root, window);
                }
            } break;
        }

        free(ev);
    }
}
