#include <xcb/xcb.h>
#include <xcb/xfixes.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

xcb_atom_t intern_atom(xcb_connection_t *conn, const char *atom) {
    xcb_atom_t result = XCB_NONE;
    xcb_intern_atom_reply_t *reply =
        xcb_intern_atom_reply(conn, xcb_intern_atom(conn, 0, strlen(atom), atom), NULL);
    if (reply) {
        result = reply->atom;
    }

    free(reply);
    return result;
}

xcb_window_t get_active_window(xcb_connection_t *conn, xcb_window_t window, xcb_atom_t atom) {
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, atom, XCB_ATOM_WINDOW, 0, 1);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
    xcb_window_t result = *(xcb_window_t*)xcb_get_property_value(reply);
    free(reply);
    return result;
}

char *get_title(xcb_connection_t *conn, xcb_window_t window) {
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, 0, window, XCB_ATOM_WM_NAME, XCB_ATOM_ANY, 0, -1);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(conn, cookie, NULL);
    const char *source_title = (char*)xcb_get_property_value(reply);
    size_t length = strlen(source_title);
    char *title = (char*)malloc(length * sizeof(char));
    memcpy(title, source_title, length);
    free(reply);
    return title;
}

void update_cursor(xcb_connection_t *conn, xcb_window_t window, char *title, char *titles[], int title_count) {
    for (int i = 1; i < title_count; i++) {
        const char* t = titles[i];
        if (strcmp(title, t) == 0) {
            xcb_xfixes_hide_cursor(conn, window);
            xcb_flush(conn);
            return;
        }
    }
    xcb_xfixes_show_cursor(conn, window);
    xcb_flush(conn);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Must include at least one window title as an argument!\n");
        exit(EXIT_FAILURE);
    }

    xcb_connection_t *conn;

    if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL)))
        return 0;

    xcb_atom_t active_window = intern_atom(conn, "_NET_ACTIVE_WINDOW");

    if (active_window == XCB_NONE) {
        fprintf(stderr, "_NET_ACTIVE_WINDOW atom not found!");
        exit(EXIT_FAILURE);
    }

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    uint32_t values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE };
    xcb_change_window_attributes(conn, screen->root, XCB_CW_EVENT_MASK, values);
    xcb_flush(conn);

    xcb_xfixes_query_version_cookie_t xfixes_cookie = xcb_xfixes_query_version(conn, 4, 0);
    xcb_xfixes_query_version_reply_t *xfixes_reply = xcb_xfixes_query_version_reply(conn, xfixes_cookie, NULL);
    if (xfixes_reply->major_version < 4) {
        fprintf(stderr,
                "Xfixes version 4.0 or greater required, you have %d.%d\n",
                xfixes_reply->major_version, xfixes_reply->minor_version);
        exit(EXIT_FAILURE);
    }
    
    do {
        xcb_window_t window = get_active_window(conn, screen->root, active_window);
        char *title = get_title(conn, window);
        update_cursor(conn, screen->root, title, argv, argc);
        free(title);
    } while (0);

    while (1) {
        xcb_generic_event_t *ev;
        ev = xcb_wait_for_event(conn);
        if (!ev) {
            fprintf(stderr, "Error in xcb event loop!");
            exit(EXIT_FAILURE);
        }

        switch (ev->response_type & ~0x80) {
            case XCB_PROPERTY_NOTIFY: {
                xcb_property_notify_event_t *e = (xcb_property_notify_event_t *)ev;
                if (e->atom == active_window) {
                    xcb_window_t window = get_active_window(conn, screen->root, active_window);
                    char *title = get_title(conn, window);
                    update_cursor(conn, screen->root, title, argv, argc);
                    free(title);
                }
            } break;
        }

        free(ev);
    }
}
