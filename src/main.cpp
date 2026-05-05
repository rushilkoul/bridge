#include <ncurses.h>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include "net/peer.hpp"

static const int SIDEBAR_W = 28;

struct Windows {
    WINDOW* header;
    WINDOW* sidebar;
    WINDOW* messages;
    WINDOW* input;
    int rows, cols;
};

static Windows create_windows(int rows, int cols) {
    Windows w;
    w.rows = rows;
    w.cols = cols;

    int main_h     = rows - 4;
    int msg_w      = cols - SIDEBAR_W - 1;

    w.header   = newwin(2,    cols,    0,          0);
    w.sidebar  = newwin(main_h, SIDEBAR_W, 2,      0);
    w.messages = newwin(main_h, msg_w,  2,          SIDEBAR_W + 1);
    w.input    = newwin(2,    cols,    rows - 2,   0);

    // apparently this fixes escape sequences. 
    // also probably use arrow keys to switch between peers
    // named constants like KEY_UP and KEY_DOWN are exposed
    // ....also i have no idea if this even works.
    keypad(w.input, TRUE);

    return w;
}

static void destroy_windows(Windows& w) {
    delwin(w.header);
    delwin(w.sidebar);
    delwin(w.messages);
    delwin(w.input);
}

static void draw_header(WINDOW* win, const std::string& my_name, int cols) {
    werase(win);

    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, 0, 0, "BRIDGE v0.1");
    wattroff(win, COLOR_PAIR(1) | A_BOLD);

    std::string login = "Logged in as " + my_name;
    int lpos = cols - (int)login.size();
    if (lpos < 0) lpos = 0;
    wattron(win, COLOR_PAIR(3) | A_DIM);
    mvwprintw(win, 0, lpos, "%s", login.c_str());
    wattroff(win, COLOR_PAIR(3) | A_DIM);

    wmove(win, 1, 0);
    for (int i = 0; i < cols; i++) waddch(win, '-');

    wrefresh(win);
}

static void draw_sidebar(WINDOW* win, int local_port, const std::vector<Connection>& conns, int max_rows)
{
    werase(win);

    int h, w;
    getmaxyx(win, h, w);
    for (int r = 0; r < h; r++) {
        wattron(win, COLOR_PAIR(3));
        mvwaddch(win, r, w - 1, ACS_VLINE);
        wattroff(win, COLOR_PAIR(3));
    }

    wattron(win, A_BOLD);
    mvwprintw(win, 0, 1, "Port:");
    wattroff(win, A_BOLD);

    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 0, 7, "%d", local_port);
    wattroff(win, COLOR_PAIR(2));

    wattron(win, A_BOLD);
    mvwprintw(win, 2, 1, "Connections");
    wattroff(win, A_BOLD);

    for (size_t i = 0; i < conns.size(); i++) {
        int row = 3 + (int)i;
        if (row >= max_rows - 2) break;
        wattron(win, COLOR_PAIR(4));
        mvwprintw(win, row, 2, "%s", conns[i].name.c_str());
        wattroff(win, COLOR_PAIR(4));
    }

    wrefresh(win);
}

static void draw_messages(WINDOW* win, const std::vector<Message>& msgs) {
    werase(win);

    int h, w;
    getmaxyx(win, h, w);
    (void)w;

    int visible = h;
    int start   = (int)msgs.size() > visible
                  ? (int)msgs.size() - visible : 0;

    for (int i = start; i < (int)msgs.size(); i++) {
        int row = i - start;
        const auto& m = msgs[i];

        int color = (m.sender == "You") ? COLOR_PAIR(4) : COLOR_PAIR(1);

        wattron(win, color | A_BOLD);
        mvwprintw(win, row, 0, "%s: ", m.sender.c_str());
        wattroff(win, color | A_BOLD);

        wprintw(win, "%s", m.text.c_str());
    }

    wrefresh(win);
}

static void draw_input(WINDOW* win, const std::string& buf, int cols) {
    werase(win);

    wmove(win, 0, 0);
    for (int i = 0; i < cols; i++) waddch(win, '-');

    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 1, 0, "You: ");
    wattroff(win, COLOR_PAIR(4) | A_BOLD);

    wprintw(win, "%s", buf.c_str());

    wrefresh(win);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: bridge <local_port> <target_ip> <target_port>\n");
        return 1;
    }

    int local_port  = std::stoi(argv[1]);
    std::string tip = argv[2];
    int target_port = std::stoi(argv[3]);
    system("clear");


    printf("Enter display name: ");
    fflush(stdout);
    std::string my_name;
    std::getline(std::cin, my_name);

    // ncurses init
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    start_color();
    use_default_colors();
    refresh();

    init_pair(1, COLOR_GREEN,  -1);
    init_pair(2, COLOR_YELLOW, -1);
    init_pair(3, COLOR_WHITE,  -1);
    init_pair(4, COLOR_CYAN,   -1);

    int rows, cols;

    getmaxyx(stdscr, rows, cols);
    Windows wins = create_windows(rows, cols);
    wmove(wins.input, 1, 5);  // row 1 (input line), col 5 (after "You: ")
    wrefresh(wins.input);  


    // actual peer setup
    Peer p(local_port, my_name);

    p.start();

    if (target_port != 0)
        p.connect(RemotePeer{tip, target_port, "unknown peer"});


    std::string input_buf;
    std::mutex  input_mutex;

    wtimeout(wins.input, -1);

    std::thread input_thread([&]() {
        while (true) {
            int ch = wgetch(wins.input);

            {
                std::lock_guard<std::mutex> lk(input_mutex);

                if (ch == '\n' || ch == KEY_ENTER) {
                    if (!input_buf.empty()) {
                        p.send_to(0, input_buf);
                        input_buf.clear();
                    }
                } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                    if (!input_buf.empty())
                        input_buf.pop_back();
                } else if (ch >= 32 && ch < 127) {
                    input_buf += (char)ch;
                }
            }

            // probably a bad way to do this, but redraw input on every keypress. 
            // i cannot figure out a better method; spent hours on just this. but anyway:
            {
                std::lock_guard<std::mutex> lk(input_mutex);
                draw_input(wins.input, input_buf, wins.cols);
            }
        }
    });
    input_thread.detach();


    while (true) {
        // resize check
        int nr, nc;
        getmaxyx(stdscr, nr, nc);
        if (nr != wins.rows || nc != wins.cols) {
            destroy_windows(wins);
            wins = create_windows(nr, nc);
            clear();
            refresh();
        }

        draw_header(wins.header, my_name, wins.cols);
        draw_sidebar(wins.sidebar, local_port, p.get_connections(), wins.rows);
        draw_messages(wins.messages, p.get_messages());

        {
            std::lock_guard<std::mutex> lk(input_mutex);
            draw_input(wins.input, input_buf, wins.cols);
        }

        napms(100); // 100ms sleep
    }

    endwin();
    return 0;
}