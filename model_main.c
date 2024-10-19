//The C main file for a ROSS model
//This file includes:
// - definition of the LP types
// - command line argument setup
// - a main function

//includes
#include "model.h"
#include "init.c"

//extern sqlite3 *db;

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (atoi(argv[1]) > best_box.row) {
        int flag = 1;
        for (int i = 0; i < MAX_ROBOTS; ++i) {
            if (Store.robots[i].reserved_channel == atoi(argv[2])) {
                flag = 0;
            }
        }
        if (flag == 1) {
            best_box.row = atoi(argv[1]);
            best_box.column = atoi(argv[2]);
        }
    }
    return 0;
}

int callback_by_width(void *NotUsed, int argc, char **argv, char **azColName) {
    if (atoi(argv[1]) > best_box.row) {
        int flag = 1;
        for (int i = 0; i < MAX_ROBOTS; ++i) {
            if (Store.robots[i].reserved_channel == atoi(argv[2])) {
                flag = 0;
            }
        }
        if (flag == 1) {
            best_box.row = atoi(argv[1]);
            best_box.column = atoi(argv[2]);
        }
    }
    return 0;
}

int compare(const void *a, const void *b) {
    const box_pair *boxA = (const box_pair *)a;
    const box_pair *boxB = (const box_pair *)b;
    return boxA->width - boxB->width;
}

int insert_data(sqlite3 **db1, int type, int row, int col, int width) {
    struct sqlite3 * db = (struct sqlite3 *) *db1;
    char *err_msg = 0;
    char sql[100];
    sprintf(sql, "INSERT INTO Warehouse(Type, Row, Column, Width, Channel_Width) VALUES (%d, %d, %d, %d, %d)", type, row, col, width, Store.conveyor_width[col]);
    sqlite3_exec(db, sql, 0, 0, &err_msg);
    return 0;
}

int find_data(sqlite3 **db1, int type) {
    struct sqlite3 * db = (struct sqlite3 *) *db1;
    best_box.row = -1;
    best_box.column = -1;
    char *err_msg = 0;
    char sql[100];
    sprintf(sql, "SELECT * FROM Warehouse WHERE Type = %d", type);
    sqlite3_exec(db, sql, callback, 0, &err_msg);
    return 0;
}

int find_data_by_width(sqlite3 **db1, int type) {
    struct sqlite3 * db = (struct sqlite3 *) *db1;
    best_box.row = -1;
    best_box.column = -1;
    char *err_msg = 0;
    char sql[100];
    // printf("%d\n", 1);
    // printf("%d %d\n", type, Store.b_w[type]);
    // sprintf(sql, "SELECT * FROM Warehouse WHERE Channel_Width >= %d AND Type = %d", Store.b_w[type], -1);
    sprintf(sql, "SELECT * FROM Warehouse WHERE Channel_Width >= %d AND Type = %d", -1, -1);
    // exit(0);
    sqlite3_exec(db, sql, callback_by_width, 0, &err_msg);
    return 0;
}

int Add_Box(sqlite3 **db1, int type, int process) {
    struct sqlite3 * db = (struct sqlite3 *) *db1;
    // find_data_by_width(db, type);
    int col = Store.robots[process - 1].col;
    int r = Store.robots[process - 1].row;
    char *err_msg = 0;
    char sql[100];
    sprintf(sql, "DELETE FROM Warehouse WHERE Type = %d AND Row = %d AND Column = %d", -1, r, col);
    sqlite3_exec(db, sql, 0, 0, &err_msg);

    Store.conveyor[col].boxes[r].SKU = type;
    Store.conveyor[col].boxes[r].empty = 0;
    Store.conveyor[col].boxes[r].width = Store.b_w[type];
    Store.cnt_boxes_type[type]++;
    insert_data(db1, type, r, col, Store.b_w[type]);
}

void Swap_Boxes(sqlite3 **db1, int col, int row1, int row2) {
    struct sqlite3 * db = (struct sqlite3 *) *db1;
    bool empty_tmp = Store.conveyor[col].boxes[row1].empty;
    int SKU_tmp = Store.conveyor[col].boxes[row1].SKU;
    int width_tmp =  Store.conveyor[col].boxes[row1].width;
    char *err_msg = 0;
    char sql[100];
    
    sprintf(sql, "DELETE FROM Warehouse WHERE Type = %d AND Row = %d AND Column = %d", Store.conveyor[col].boxes[row1].SKU, row1, col);
    sqlite3_exec(db, sql, 0, 0, &err_msg);
    char sql2[100];
    sprintf(sql2, "DELETE FROM Warehouse WHERE Type = %d AND Row = %d AND Column = %d", Store.conveyor[col].boxes[row2].SKU, row2, col);
    sqlite3_exec(db, sql2, 0, 0, &err_msg);

    Store.conveyor[col].boxes[row1].empty = Store.conveyor[col].boxes[row2].empty;
    Store.conveyor[col].boxes[row1].SKU = Store.conveyor[col].boxes[row2].SKU;
    Store.conveyor[col].boxes[row1].width = Store.conveyor[col].boxes[row2].width;
    Store.conveyor[col].boxes[row2].empty = empty_tmp;
    Store.conveyor[col].boxes[row2].SKU = SKU_tmp;
    Store.conveyor[col].boxes[row2].width = width_tmp;

    insert_data(db1, Store.conveyor[col].boxes[row2].SKU, row2, col, Store.conveyor[col].boxes[row2].width);
    insert_data(db1, Store.conveyor[col].boxes[row1].SKU, row1, col, Store.conveyor[col].boxes[row1].width);
}

int Reverse(sqlite3 **db1, int col, int row, int *time, int *l_id, int process) {
    //fprintf(f, "%*d   %*d   getbox%*d   shiftbox%*d    channelwidth%*d    putbox%*d  channel%*d         ", 4, *l_id, 4, *time, 5, Store.conveyor[col].boxes[7].SKU, 6, Store.conveyor[col].boxes[7].SKU, 2, Store.conveyor_width[col], 2, Store.conveyor[col].boxes[7].SKU, 2, col);
    *l_id += 1;
    for (int i = MAX_BOXES - 1; i >= 1; --i) {
        if (Store.conveyor[col].boxes[i - 1].empty == 0) {
            Swap_Boxes(db1, col, i, i - 1);
        }
    }
    Store.robots[process - 1].row += 1;
    return 0;
}

int Remove_Boxes(sqlite3 **db1, int type, int *time, int *l_id, int process) {
    int col = Store.robots[process - 1].col;
    int row = Store.robots[process - 1].row;
    struct sqlite3 * db = (struct sqlite3 *) *db1;

    char *err_msg = 0;
    char sql[100];
    sprintf(sql, "DELETE FROM Warehouse WHERE Type = %d AND Row = %d AND Column = %d", type, 7, col);
    sqlite3_exec(db, sql, 0, 0, &err_msg);

    insert_data(db1, -1, MAX_BOXES - 1, col, -1);

    Store.conveyor[col].boxes[MAX_BOXES - 1].SKU = -1;
    Store.conveyor[col].boxes[MAX_BOXES - 1].empty = 1;
    Store.conveyor[col].boxes[MAX_BOXES - 1].width = -1;

    for (int i = MAX_BOXES - 1; i >= 1; --i) {
        Swap_Boxes(db1, col, i, i - 1);
    }
    Store.cnt_boxes_type[type]--;

    return col;
}

void Init_Commands(int *event_id, int *rec_id, int *glb_time, const char *filename) {
    FILE *file1 = fopen(filename, "r");
    int req_num = 0;
    Store.request.total = 0;
    Store.request.curr = 0;

    char line[1024];
    char *fields[10];

    fgets(line, sizeof(line), file1);

    char *temp_line = strtok(line, ",");

    strncpy(Store.cur_order, temp_line, sizeof(Store.cur_order) - 1);

    // Store.cur_order = strtok(line, ",");
    fprintf(paleta, "%*d %*d %*s %*s", 6, *rec_id, 6, *glb_time, 18, "startPalletize", 22, strtok(line, ","));
    fprintf(paleta, "\n");
    (*rec_id) += 1;

    fprintf(f, "%*d %*d       startPalletize %s", 6, *event_id, 6, *glb_time, strtok(line, ","));
    (*event_id) += 1;
    fgets(line, sizeof(line), file1);

    while (fgets(line, sizeof(line), file1)) {
        fields[0] = strtok(line, ",");
        for (int i = 1; i < 10; i++) {
          fields[i] = strtok(NULL, ",");
        }
        int SKU =  atoi(fields[0]) - 1000;
        int quantity = atoi(fields[1]);
        while (quantity != 0) {
          Store.request.requests[req_num][0] = SKU;
          Store.request.requests[req_num][1] = 1;
          quantity--;
          Store.request.total++;
          req_num++;
        }
    }
}

bool Check(int process) {
    if (Store.request.curr == Store.request.total) {
        return false;
    } else {
        int SKU = Store.request.requests[Store.request.curr][0];
        int quantity = Store.request.requests[Store.request.curr][1];
        Store.box_data[process][0] = SKU;
        Store.box_data[process][1] = quantity;
        Store.request.curr++;
        return true;
    }
}

void Send_Event(int process, message_type command, tw_lp *lp, tw_lpid *self) {
    tw_event *e = tw_event_new(process, 0.001, lp);
    message *msg = tw_event_data(e);
    msg->type = command;
    msg->contents = tw_rand_unif(lp->rng);
    msg->sender = self;
    tw_event_send(e);
}


void Print_Channel(int col, FILE *log_file) {
    fprintf(f, "%d      ", col);
    for (int i = 0; i < MAX_BOXES; ++i) {
        if (Store.conveyor[col].boxes[i].empty) {
            fprintf(log_file, "| - ");
        } else {
            fprintf(log_file, "|%*d", 3, Store.conveyor[col].boxes[i].SKU);
        }
    }
    fprintf(log_file, "|\n");
}


void write_csv(const char *filename, sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT * FROM Warehouse"; // Замените your_table на имя вашей таблицы

    // Подготовка запроса
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Ошибка при подготовке SQL запроса: %s\n", sqlite3_errmsg(db));
        return;
    }

    if (!csv_file) {
        fprintf(stderr, "Не удалось открыть файл для записи: %s\n", filename);
        sqlite3_finalize(stmt);
        return;
    }

    // Запись заголовков
    int num_columns = sqlite3_column_count(stmt);
    for (int i = 0; i < num_columns; i++) {
        printf("%s\n", sqlite3_column_name(stmt, i));
        fprintf(csv_file, "%s", sqlite3_column_name(stmt, i));
        if (i < num_columns - 1) {
            fprintf(csv_file, ",");
        }
    }
    fprintf(csv_file, "\n");

    // Запись данных
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int i = 0; i < num_columns; i++) {
            const char *value = (const char *)sqlite3_column_text(stmt, i);
            int to_print = 0;
            if (i == 2) {
                to_print = atoi(value);
                fprintf(csv_file, "%d", to_print % 10 + 1);
            } else {
                if (value) {
                    fprintf(csv_file, "%s", value);
                }
            }
            if (i < num_columns - 1) {
                fprintf(csv_file, ",");
            }
        }
        fprintf(csv_file, "\n");
    }

    fprintf(robots_positions, "BotID");
    fprintf(robots_positions, ", ");
    fprintf(robots_positions, "BotNode");
    fprintf(robots_positions, ", ");
    fprintf(robots_positions, "BoxTypeID");
    fprintf(robots_positions, "\n");
    for (int i = 0; i < MAX_ROBOTS; ++i) {
        fprintf(robots_positions, "%d", i);
        fprintf(robots_positions, ",     ");
        fprintf(robots_positions, "%s", Store.vertexes[Store.robots[i].cur_cell.id]);
        fprintf(robots_positions, ",      ");
        fprintf(robots_positions, "%d", Store.robots[i].cur_box);
        fprintf(robots_positions, "\n");    
    }

    fclose(csv_file);
    sqlite3_finalize(stmt);
}

int next_vertex(int cur_vertex, int cur_goal) {
    if (Store.vertexes[cur_goal][0] == Store.vertexes[cur_vertex][0]) {  // если нам нужно двигаться в текущей строке
        if (Store.vertexes[cur_vertex + 1][0] != Store.vertexes[cur_vertex][0]) { // если цель в обратном направлении 
            return Store.direction_graph[cur_vertex];
        } else {
            return cur_vertex + 1;
        }
    } else if (Store.vertexes[cur_goal][0] > Store.vertexes[cur_vertex][0]) { // если нам нужно двигаться вверх
        if (Store.direction_graph[cur_vertex] != -1 && Store.vertexes[Store.direction_graph[cur_vertex]][1] != '1') { // если мы на стыке и можно вверх
            return Store.direction_graph[cur_vertex];
        } else if (Store.direction_graph[cur_vertex] == -1) { // если мы не на стыке
            return cur_vertex + 1;
        } else { // иначе вниз
            return Store.direction_graph[cur_vertex];
        }
    } else {
        if (Store.direction_graph[cur_vertex] != -1 && Store.vertexes[Store.direction_graph[cur_vertex]][1] == '1') { // если мы на стыке и можно вниз
            return Store.direction_graph[cur_vertex];
        } else if (Store.direction_graph[cur_vertex] == -1) { // если мы не на стыке
            return cur_vertex + 1;
        } else { // иначе вверх
            return Store.direction_graph[cur_vertex];
        }
    }
}

void add_to_queue(int robot_id, int next_vert) {

    // if ((Store.robots[robot_id].goal_cell.id == Store.robots[robot_id].cur_cell.id) || (Store.cells[next_vert].queue[0] != -1)) {

        // fprintf(f, "BEFORE ADDING ID %d, CELL %s ", robot_id + 1, Store.vertexes[next_vert]);
        // for (int j = 0; j < MAX_ROBOTS; ++j) {
        //     fprintf(f, "%d ", Store.cells[next_vert].queue[j] + 1);
        // }
        // fprintf(f, "\n");

    for (int i = 0; i < MAX_ROBOTS; ++i) {
        if (Store.cells[next_vert].queue[i] == -1) {
            Store.cells[next_vert].queue[i] = robot_id;
            // fprintf(f, "AFTER ADDING ");
            // for (int j = 0; j < MAX_ROBOTS; ++j) {
            //     fprintf(f, "%d ", Store.cells[next_vert].queue[j] + 1);
            // }
            // fprintf(f, "\n");
            return;
        }
        }

    // }
}

void del_from_queue(int robot_id) {
    if (robot_id == Store.cells[Store.robots[robot_id].cur_cell.id].queue[0]) {

        // fprintf(f, "BEFORE DELETING ID %d, CELL %s ", robot_id + 1, Store.vertexes[Store.robots[robot_id].cur_cell.id]);
        // for (int j = 0; j < MAX_ROBOTS; ++j) {
        //     fprintf(f, "%d ", Store.cells[Store.robots[robot_id].cur_cell.id].queue[j] + 1);
        // }
        // fprintf(f, "\n");

        Store.cells[Store.robots[robot_id].cur_cell.id].queue[0] = -1;
        for (int i = 0; i < MAX_ROBOTS - 1; ++i) {
            int temp = Store.cells[Store.robots[robot_id].cur_cell.id].queue[i];
            Store.cells[Store.robots[robot_id].cur_cell.id].queue[i] =  Store.cells[Store.robots[robot_id].cur_cell.id].queue[i + 1];
            Store.cells[Store.robots[robot_id].cur_cell.id].queue[i + 1] = temp;
        }

        // fprintf(f, "AFTER DELETING ");
        // for (int j = 0; j < MAX_ROBOTS; ++j) {
        //     fprintf(f, "%d ", Store.cells[Store.robots[robot_id].cur_cell.id].queue[j] + 1);
        // }
        // fprintf(f, "\n");

    }
}

int GeTrIdFromBot(int curr_cell) {
  if (curr_cell < 3) {
    return curr_cell + 3;
  }
  if (curr_cell + 1 == MAX_VERTEXES - 2) {
    return 2;
  }
  if (curr_cell + 1 == MAX_VERTEXES - 1) {
    return 1;
  }
}

int CellIdFromName(char* name) {
  int code = (int)(name[5] - 'A');
  if (code == 0) {
    return (int)(name[7] - '0') - 1;
  }
  if (code % 2 == 0) {
    return (int)(name[7] - '0') + 6 + (code - 1) * 6;
  }
  return 7 + code * 6 - (int)(name[7] - '0');
}

void init_graph() {
    for (int i = 0; i < 49; ++i) {
        Store.direction_graph[i] = -1;
    }

    Store.direction_graph[6] = 7;

    Store.direction_graph[7] = 18;

    Store.direction_graph[18] = 19;

    Store.direction_graph[19] = 30;

    Store.direction_graph[30] = 31;

    Store.direction_graph[31] = 42;

    Store.direction_graph[42] = 43;

    Store.direction_graph[48] = 37;

    Store.direction_graph[37] = 36;

    Store.direction_graph[36] = 25;

    Store.direction_graph[25] = 24;

    Store.direction_graph[24] = 13;

    Store.direction_graph[13] = 12;

    Store.direction_graph[12] = 0;
}

void InitVertexNames(char ch, int from, int to) {
    if (from < to) {
        int ind = 1;
        while (from - 1 <= to) {
            char str1[5];
            str1[0] = ch;
            str1[1] = '\0';
            char str2[3];
            sprintf(str2, "%d", ind); 
            strcat(str1, str2);
            strcpy(Store.vertexes[from - 1], str1);
            from++;
            ind++;
        }
    } else {
        int ind = 1;
        while (from > to) {
            char str1[5]; 
            str1[0] = ch;
            str1[1] = '\0';
            char str2[3];
            sprintf(str2, "%d", ind); 
            strcat(str1, str2);
            strcpy(Store.vertexes[from - 1], str1); 
            from--;
            ind++;
        }
    }
}

void ConveyorsInit()
{
    // Store.type_to_add = -1;
    char *err_msg = 0;
    char *sql_del = "DROP TABLE IF EXISTS Warehouse";
    char *sql = "CREATE TABLE Warehouse(Type INTEGER, Row INTEGER, Column INTEGER, Width INTEGER, Channel_Width INTEGER)";
    sqlite3_exec(Store.db, sql_del, 0, 0, &err_msg);
    sqlite3_exec(Store.db, sql, 0, 0, &err_msg);

    int values[] = {51, 43, 46, 46, 43, 46, 46, 38, 33, 41, 38, 33, 33, 30, 64, 30, 23, 20, 17, 17, 15, 10, 7, 7, 7, 7, 20, 7, 12, 7, 23, 10, 7, 10, 7, 7, 7, 7, 7, 7, 7, 12, 7, 7, 10, 7, 7, 10, 5, 2};
    int distribution[] = {20, 17, 18, 18, 17, 18, 18, 15, 13, 16, 15, 13, 13, 12, 25, 12, 9, 8, 7, 7, 6, 4, 3, 3, 3, 3, 8, 3, 5, 3, 9, 4, 3, 4, 3, 3, 3, 3, 3, 3, 3, 5, 3, 3, 4, 3, 3, 4, 2, 1};
    for (int i = 1; i < high_border - low_border + 1; ++i) {
      Store.thrs_for_each_sku[i] = distribution[i - 1] + distribution[i - 1] / 2;
    // printf("%d ", Store.thrs_for_each_sku[i]);
      Store.max_boxes_for_each_sku[i] = values[i - 1];
    }

    init_graph();

    InitVertexNames('A', 1, MAX_RACKS + 1);
    InitVertexNames('B', MAX_RACKS * 2 + 3, MAX_RACKS + 2);
    InitVertexNames('C', MAX_RACKS * 2 + 4, MAX_RACKS * 3 + 3);
    InitVertexNames('D', MAX_RACKS * 5, MAX_RACKS * 3 + 4);
    InitVertexNames('E', MAX_RACKS * 5 + 1, MAX_RACKS * 6);
    InitVertexNames('F', MAX_RACKS * 7 + 2, MAX_RACKS * 6 + 1);
    InitVertexNames('G', MAX_RACKS * 7 + 3, MAX_RACKS * 8 + 2);
    InitVertexNames('H', MAX_RACKS * 9 + 4, MAX_RACKS * 8 + 3);
    Store.vertexes[MAX_VERTEXES][0] = '#';

    Store.kill_prog = false;

    for (int i = 0; i < MAX_CONVEYORS; ++i) {
       // Store.conveyor_width[i] = i % 5 + 1;
        Store.conveyor_width[i] = 5;
    }


    for (int i = 0; i < high_border - low_border + 1; ++i) {
        Store.b_w[i] = i % 5;
        // printf("%d\n", Store.b_w[i]);
        box_pair bp;
        bp.SKU = i;
        bp.width = i % 5;
        // bp.width = 1
        Store.box_width[i] = bp;

    }

    // Initialization of cells
    for (int i = 0; i < MAX_VERTEXES; ++i) {
        // printf("%d\n", i);
        cell c_cell;
        for (int j = 0; j < MAX_ROBOTS; ++j) {
            c_cell.queue[j] = -1;
        }
        c_cell.id = i;
        c_cell.reserved = 0;
        Store.cells[i] = c_cell;
    }


    char line[256];
    char *fields[3];
    fgets(line, sizeof(line), bots_starting_positions);
    
    for (int i = 0; i < MAX_ROBOTS; ++i) {

        fgets(line, sizeof(line), bots_starting_positions);
        fields[0] = strtok(line, ",");
        fields[1] = strtok(NULL, ",");

        Store.messages[i].type = GO;
        robot bot;
        bot.tmp_fl = 1;
        bot.cur_task = -1;
        bot.cur_box = -1;
        bot.pre_reserved = -1;
        bot.low_SKU = -1;
        bot.col = -1;
        bot.row = -1;
        bot.kill = 0;
        bot.has_box = -1;
        bot.reserved_channel = -1;
        bot.cur_time = 0;
        bot.goal_time = 0;
        bot.cur_cell = Store.cells[CellIdFromName(fields[1])];
        bot.prev_vertex = -1;
        bot.prev_box_type = -1;
        bot.prev_channel = -1;
        bot.prev_tr_id = -1;
        Store.robots[i] = bot;    
    }

    

    int change_tmp = 0;
    for (int i = 0; i < high_border - low_border + 1; ++i) {
        Store.cnt_boxes_type[i] = 0;
        Store.cnt_boxes_type_const[i] = 0;
    }

    for (int row = 0; row < MAX_BOXES; ++row) {
        for (int col = 0; col < MAX_CONVEYORS; ++col) {
            box b;
            b.empty = 1;
            b.SKU = -1;
            b.width = -1;
            Store.conveyor[col].boxes[row] = b;
        }
    }
    int id = 1;
    for (int row = MAX_BOXES - 1; row >= 0; --row) {
        for (int col = 0; col < MAX_CONVEYORS; ++col) {
            int current_SKU = (low_border + (change_tmp) % (high_border - low_border));
            Store.conveyor[col].boxes[row].SKU = -1;
            Store.conveyor[col].boxes[row].width = -1;
            Store.conveyor[col].boxes[row].empty = 1;
            change_tmp++;
            
            insert_data(&(Store.db), -1, row, col, -1);
        }
    }
    
    for (int i = 0; i < MAX_ROBOTS + 1; ++i) {
        Store.used[i] = 0;
    }
    Store.boxes_to_deliver = 0;
    Store.cur_file = 0;

    for (int i = 0; i < MAX_ROBOTS + 1; ++i) {
        Store.box_data[i][0] = -1;
        Store.box_data[i][1] = 0;
    }
}

void InitROSS() {
    ConveyorsInit();

}

//The C driver file for a ROSS model
//This file includes:
// - an initialization function for each LP type
// - a forward event function for each LP type
// - a reverse event function for each LP type
// - a finalization function for each LP type

//Includes
//Helper Functions
void SWAP (double *a, double *b) {
  double tmp = *a;
  *a = *b;
  *b = tmp;
}

void model_init (state *s, tw_lp *lp) {
  int self = lp->gid;
  tw_event *e = tw_event_new(0, 1, lp);
  message *msg = tw_event_data(e);
  msg->type = TAKE_OUT;
  msg->contents = tw_rand_unif(lp->rng);
  msg->sender = self;
  tw_event_send(e);
  if (self == 0) {
    printf("%s\n", "COMMAND_CENTER is initialized");
  } else {
    printf("%s ", "CONVEYOR");
    printf("%d ", self);
    printf("%s\n", " is initialized");
  }
}

void model_event (state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
  int self = lp->gid;
  *(int *) bf = (int) 0;
  SWAP(&(s->value), &(in_msg->contents));

  Store.kill_prog = 0;
  int ok_take_in = 1;
  for (int i = 0; i < MAX_ROBOTS; ++i) {
    if (Store.robots[i].kill == 1) {
      Store.kill_prog = 1;
      ok_take_in = 0;
    }
  }

  if (self == 0 && Store.kill_prog == 0) {
    int not_do = 0;
    for (int i = 1; i < MAX_ROBOTS + 1; ++i) {
      if (Store.used[i] == 1) {
        not_do = 1;
      }
    }
    
    if (not_do == 0) {
      glb_time += 1;
      if (Store.boxes_to_deliver <= 0) {
        for (int i = 1; i < high_border - low_border + 1; ++i) {
          if (ok_take_in && Store.cnt_boxes_type[i] <= Store.thrs_for_each_sku[i]) {
            printf("SUKA %d\n", i);
            for (int j = 1; j < high_border - low_border + 1; ++j) {
               printf("%d ", Store.cnt_boxes_type[j]);
            }
            printf("\n");

            Store.boxes_to_deliver = Store.max_boxes_for_each_sku[i] - Store.cnt_boxes_type[i];
            Store.type_to_add = i;

            
            for (int cur_robot = 0; cur_robot < MAX_ROBOTS; ++cur_robot) {
              if (Store.robots[i - 1].cur_task != 3 && (Store.robots[cur_robot].cur_task == 1 || Store.boxes_to_deliver >= cur_robot + 1) && Store.robots[cur_robot].cur_task != 2) {
                //printf("1");
                Store.nt_used[cur_robot + 1] = 1;
                Store.used[cur_robot + 1] = 1;
                Store.robots[cur_robot].cur_task = 1;
                Store.messages[cur_robot].type = TAKE_IN;
                if (Store.robots[cur_robot].has_box == -1) {
                  srand(time(NULL));
                  Store.robots[cur_robot].goal_cell.id = 46 + (int)(rand() % 2);
                }
              }
            }
            //printf("\n");
  

            // printf("%d %d\n", Store.type_to_add, Store.boxes_to_deliver);

            
            fprintf(paleta, "%*d %*d %*s %*d %*d", 6, rec_id, 6, glb_time, 18, "StartDepalletize", 21, palet_type, 12, Store.type_to_add);
            fprintf(paleta, "\n");
            // palet_type += 1;
            rec_id++;
            fprintf(f, "%*d %*d     startDepalletize %d\n", 6, event_id, 6, glb_time, Store.type_to_add);
            fprintf(control_system_log, "%*d %*d     startDepalletize %d\n", 6, control_id, 6, glb_time, Store.type_to_add);
            control_id += 1;
            event_id += 1;
            break;
          }
        }
      } else {

        //printf("%d\n", Store.boxes_to_deliver);

        for (int cur_robot = 0; cur_robot < MAX_ROBOTS; ++cur_robot) {
          if (Store.robots[cur_robot].cur_task != 3 && ok_take_in && (Store.robots[cur_robot].cur_task == 1 || Store.boxes_to_deliver >= cur_robot + 1) && Store.robots[cur_robot].cur_task != 2) {
            Store.nt_used[cur_robot + 1] = 1;
            Store.used[cur_robot + 1] = 1;
            Store.robots[cur_robot].cur_task = 1;
            Store.messages[cur_robot].type = TAKE_IN;
            if (Store.robots[cur_robot].has_box == -1) {
              srand(time(NULL));
              Store.robots[cur_robot].goal_cell.id = 46 + (int)(rand() % 2);
            }
          }
        }
      }
      // printf("%d\n", cur_boxes);

      for (int process = 1; process < MAX_ROBOTS + 1; ++process) {
        if (Store.robots[process - 1].cur_task != 3 && cur_boxes >= 1000 && Store.robots[process - 1].cur_task != 1) {
          printf("\n BROKE HERE");
          Store.nt_used[process] = 1;
          if (Store.robots[process - 1].col != -1 && Store.robots[process - 1].row != -1) {
            Store.robots[process - 1].cur_task = 2;
            if (Store.robots[process - 1].row != 7) {
              Store.used[process] = 1;
              Store.messages[process - 1].type = REVERSE;
            } else {
              Store.used[process] = 1;
              Store.messages[process - 1].type = TAKE_OUT;
            }
          } else {
            if (Check(process)) {
              Store.robots[process - 1].cur_task = 2;
              find_data(&(Store.db), Store.box_data[process][0]);
              Store.robots[process - 1].goal_cell.id = (int)(best_box.column / 10) + 7 + (int)(best_box.column / 50) * 7;
              Store.robots[process - 1].reserved_channel = best_box.column;

              Store.robots[process - 1].col = best_box.column;
              Store.robots[process - 1].row = best_box.row;

              if (Store.robots[process - 1].row != 7) {
                Store.used[process] = 1;
                Store.messages[process - 1].type = REVERSE;
              } else {
                Store.used[process] = 1;
                Store.messages[process - 1].type = TAKE_OUT;
              }
            } else {
              if (Store.cur_file > MAX_FILES) {
                Store.robots[process - 1].kill = 1;
              } else {
                if (Store.cur_file != 0) {
                  fprintf(paleta, "%*d %*d %*s %*s", 6, rec_id, 6, glb_time, 18, "finishPalletize", 21, Store.cur_order);
                  fprintf(paleta, "\n");
                  rec_id++;
                  fprintf(f, "%*d %*d      finishPalletize %s", 6, event_id, 6, glb_time, Store.cur_order);
                  event_id += 1;
                }
                Init_Commands(&(event_id), &(rec_id), &(glb_time), Store.files[Store.cur_file]);
                Store.cur_file += 1;
              }
            }
          }

        }
      }
      
      for (int i = 1; i < MAX_ROBOTS + 1; ++i) {
        if (Store.robots[i - 1].cur_task == -1 && cur_boxes < 1000 && ok_take_in && Store.nt_used[i] == 0) {
          if (Store.robots[i - 1].cur_task != 3) {
            if (Store.robots[i - 1].cur_cell.id == 44) {
              Store.robots[i - 1].goal_cell.id = 4;
            } else {  
              Store.robots[i - 1].goal_cell.id = 44;
            }
          }
          Store.robots[i - 1].cur_task = 3;
          Store.used[i] = 1;
          Store.messages[i - 1].type = GO;
        }
      }

      Send_Event(1, Store.messages[0].type, lp, &(lp->gid));

    }
  } else if (Store.kill_prog == 0) {
    Store.used[self] = 0;
    Store.nt_used[self] = 0;

    Store.robots[self - 1].cur_time += 1;


    switch (in_msg->type)
    {
      case TAKE_IN:
        if (Store.robots[self - 1].has_box == -1 && Store.robots[self - 1].cur_cell.id == Store.robots[self - 1].goal_cell.id) {
          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, Store.robots[self - 1].cur_cell.id + 1);
            del_from_queue(self - 1);
            Store.robots[self - 1].goal_time = 8;
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            fprintf(f, "%*d %*d %*d     startMotionTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
          }
          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            find_data_by_width(&(Store.db), Store.type_to_add);


            if (best_box.row == -1 || best_box.column == -1) {

              cur_boxes += 1;
              if (self - 1 == Store.cells[Store.robots[self - 1].cur_cell.id].queue[0]) {
                  Store.cells[Store.robots[self - 1].cur_cell.id].queue[0] = -1;
                  for (int i = 0; i < MAX_ROBOTS - 1; ++i) {
                      int temp = Store.cells[Store.robots[self - 1].cur_cell.id].queue[i];
                      Store.cells[Store.robots[self - 1].cur_cell.id].queue[i] =  Store.cells[Store.robots[self - 1].cur_cell.id].queue[i + 1];
                      Store.cells[Store.robots[self - 1].cur_cell.id].queue[i + 1] = temp;
                  }
              }
              
              if (self - 1 == Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0]) {
                  Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] = -1;
                  for (int i = 0; i < MAX_ROBOTS - 1; ++i) {
                      int temp = Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[i];
                      Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[i] =  Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[i + 1];
                      Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[i + 1] = temp;
                  }
              }


              Store.robots[self - 1].cur_time = 0;
              Store.robots[self - 1].cur_task = -1;
              Store.robots[self - 1].col = -1;
              Store.robots[self - 1].row = -1;
              Store.robots[self - 1].reserved_channel = -1;
              Store.boxes_to_deliver--;
              Store.robots[self - 1].tmp_fl = 1;
              if (Store.boxes_to_deliver == 0) {
                fprintf(paleta, "%*d %*d %*s %*d %*d", 6, rec_id, 6, glb_time, 18, "finishDepalletize", 21, palet_type, 12, Store.type_to_add);
                fprintf(paleta, "\n");
                palet_type++;
                rec_id++;
                fprintf(f, "%*d %*d    finishDepalletize\n", 6, event_id, 6, glb_time);
                // Store.type_to_add = -1;
                event_id += 1;
              }

              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_IN, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }

            Store.robots[self - 1].col = best_box.column;
            Store.robots[self - 1].row = best_box.row;

            Store.robots[self - 1].reserved_channel = best_box.column;

            if (Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != -1 && Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_IN, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }

            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;

            Store.robots[self - 1].has_box = 1;
            Store.robots[self - 1].goal_cell.id = (((Store.robots[self - 1].col / 50) + 1) * 12) + (4 - ((Store.robots[self - 1].col - 50 * (Store.robots[self - 1].col / 50)) / 10)) + 1;
            fprintf(f, "%*d %*d %*d     movebox2botTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.type_to_add, 4, 0, 2, GeTrIdFromBot(Store.robots[self - 1].cur_cell.id));
            fprintf(control_system_log, "%*d %*d %*d     movebox2bot       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.type_to_add, 4, 0, 2, GeTrIdFromBot(Store.robots[self - 1].cur_cell.id));
            event_id += 1;
            control_id += 1;

            //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.type_to_add, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            Store.robots[self - 1].prev_box_type = Store.type_to_add;
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id += 1;
            //event_id += 1;

          }


        } else if (Store.robots[self - 1].has_box == 1 && Store.robots[self - 1].cur_cell.id == Store.robots[self - 1].goal_cell.id) {
          Store.robots[self - 1].goal_time = 8;
          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, Store.robots[self - 1].cur_cell.id + 1);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            fprintf(f,"%*d %*d %*d     startMotionTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.type_to_add, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            fprintf(control_system_log,"%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.type_to_add, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
          }
          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {

            if (Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != -1 && Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_IN, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }

            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;
            Add_Box(&(Store.db), Store.type_to_add, self);
            Store.boxes_to_deliver--;
            cur_boxes += 1;
            fprintf(f, "%*d %*d %*d movebox2channelTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.type_to_add, 4, Store.robots[self - 1].col, 2, 0);
            Print_Channel(Store.robots[self - 1].col, f);          
            fprintf(control_system_log, "%*d %*d %*d movebox2channel       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.type_to_add, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
            //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            Store.robots[self - 1].prev_box_type = 0;
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            
            //event_id += 1;
            Store.robots[self - 1].reserved_channel = -1;
            Store.robots[self - 1].has_box = -1;
            Store.robots[self - 1].col = -1;
            Store.robots[self - 1].row = -1;
            Store.robots[self - 1].cur_task = -1;
            if (Store.boxes_to_deliver == 0) {
              fprintf(paleta, "%*d %*d %*s %*d %*d", 6, rec_id, 6, glb_time, 18, "finishDepalletize", 21, palet_type, 12, Store.type_to_add);
              fprintf(paleta, "\n");
              rec_id++;
              palet_type++;
              fprintf(f, "%*d %*d    finishDepalletize\n", 6, event_id, 6, glb_time);
              // Store.type_to_add = -1;
              event_id += 1;
            }

            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id += 1;
          }

        } else {
          int next_vert = next_vertex(Store.robots[self - 1].cur_cell.id, Store.robots[self - 1].goal_cell.id);

          if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && Store.direction_graph[next_vert] != -1) { // если оба стыка
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && (next_vert == 0 || next_vert == 43)) { // если стык и конечная
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 0) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 1) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 4;
          } else {
            Store.robots[self - 1].goal_time = 6;
          }

          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, next_vert);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            if (Store.robots[self - 1].has_box == 1) {
              fprintf(f, "%*d %*d %*d     startMotionTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.type_to_add, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.type_to_add, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            } else {
              fprintf(f, "%*d %*d %*d     startMotionTAKE_IN       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            }
            event_id += 1;
            control_id += 1;
          }

          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            if (Store.cells[next_vert].queue[0] != -1 && Store.cells[next_vert].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_IN, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }
            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;
            if (Store.robots[self - 1].has_box == 1) {
              //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.type_to_add, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              Store.robots[self - 1].prev_box_type = Store.type_to_add;
              Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
              Store.robots[self - 1].prev_tr_id = 0;
            } else {
              //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert],  4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              Store.robots[self - 1].prev_box_type = 0;
              Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
              Store.robots[self - 1].prev_tr_id = 0;
            }
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            //event_id += 1;

            del_from_queue(self - 1);

            Store.robots[self - 1].cur_cell.id = next_vert;

          }

        }
        
        
        if (self == MAX_ROBOTS) {
          Send_Event(0, TAKE_IN, lp, &(lp->gid));
        } else {
          Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
        }
        break;

      case TAKE_OUT:

        if (Store.robots[self - 1].has_box == -1 && Store.robots[self - 1].cur_cell.id == Store.robots[self - 1].goal_cell.id) {
          
          Store.robots[self - 1].goal_time = 8;
          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, Store.robots[self - 1].cur_cell.id + 1);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            fprintf(f, "%*d %*d %*d     startMotionTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
          }
          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            if (Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != -1 && Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_OUT, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }

            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;
            Remove_Boxes(&(Store.db), Store.box_data[self][0], &(glb_time), &(event_id), self);
            Store.robots[self - 1].cur_box = Store.box_data[self][0];
            fprintf(f, "%*d %*d %*d     movebox2botTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.box_data[self][0], 4, Store.robots[self - 1].col, 2, 0);
            Print_Channel(Store.robots[self - 1].col, f);
            fprintf(control_system_log, "%*d %*d %*d     movebox2bot       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.box_data[self][0], 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;

            Store.box_data[self][1] = 0;
            Store.robots[self - 1].has_box = 1;
            Store.robots[self - 1].reserved_channel = -1;
            srand(time(NULL));
            Store.robots[self - 1].goal_cell.id = (int)(rand() % 3);

            //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.box_data[self][0], 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            Store.robots[self - 1].prev_box_type = Store.box_data[self][0];
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            //event_id += 1;

            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id += 1;
          }

        } else if (Store.robots[self - 1].has_box == 1 && Store.robots[self - 1].cur_cell.id == Store.robots[self - 1].goal_cell.id) {
          
          Store.robots[self - 1].goal_time = 8;
          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, Store.robots[self - 1].cur_cell.id + 1);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            fprintf(f, "%*d %*d %*d     startMotionTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.box_data[self][0], 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.box_data[self][0], 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
          }
          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            if (Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != -1 && Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_OUT, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }
            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;
            Store.robots[self - 1].has_box = -1;
            Store.robots[self - 1].row = -1;
            Store.robots[self - 1].col = -1;
            Store.robots[self - 1].cur_task = -1;
            fprintf(f, "%*d %*d %*d      movebox2trTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.box_data[self][0], 4, 0, 2, GeTrIdFromBot(Store.robots[self - 1].cur_cell.id));
            fprintf(control_system_log, "%*d %*d %*d      movebox2tr       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.box_data[self][0], 4, 0, 2, GeTrIdFromBot(Store.robots[self - 1].cur_cell.id));
            event_id += 1;
            control_id += 1;
            //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            Store.robots[self - 1].prev_box_type = 0;
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            //event_id += 1;

            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id += 1;
          }
        } else {
          
          int next_vert = next_vertex(Store.robots[self - 1].cur_cell.id, Store.robots[self - 1].goal_cell.id);

          if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && Store.direction_graph[next_vert] != -1) { // если оба стыка
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && (next_vert == 0 || next_vert == 43)) { // если стык и конечная
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 0) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 1) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 4;
          } else {
            Store.robots[self - 1].goal_time = 6;
          }


          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, next_vert);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            if (Store.robots[self - 1].has_box == 1) {
              fprintf(f, "%*d %*d %*d     startMotionTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.box_data[self][0], 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.box_data[self][0], 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            } else {
              fprintf(f, "%*d %*d %*d     startMotionTAKE_OUT       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            }
            event_id += 1;
            control_id += 1;
          }

          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            if (Store.cells[next_vert].queue[0] != -1 && Store.cells[next_vert].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_IN, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }
            
            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;
            if (Store.robots[self - 1].has_box == 1) {
              Store.robots[self - 1].prev_box_type = Store.box_data[self][0];
              Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
              Store.robots[self - 1].prev_tr_id = 0;
              //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert],  4, Store.box_data[self][0], 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            } else {
              Store.robots[self - 1].prev_box_type = 0;
              Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
              Store.robots[self - 1].prev_tr_id = 0;
              //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert],  4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            }
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            //event_id += 1;
            
            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id = next_vert;
            
          }
        }
        
        if (self == MAX_ROBOTS) {
          Send_Event(0, TAKE_OUT, lp, &(lp->gid));
        } else {
          Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
        }
        break;
        
      case REVERSE:

        if (Store.robots[self - 1].has_box == -1 && Store.robots[self - 1].cur_cell.id == Store.robots[self - 1].goal_cell.id) {
          
          Store.robots[self - 1].goal_time = 8;
          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, Store.robots[self - 1].cur_cell.id + 1);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }

            fprintf(f, "%*d %*d %*d     startMotionREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
          }
          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            if (Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != -1 && Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, REVERSE, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }

            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;
            Store.robots[self - 1].low_SKU = Store.conveyor[Store.robots[self - 1].col].boxes[7].SKU;
            rev_quantity += 1;
            if (rev_quantity % 100 == 0) {
              printf("%d\n", rev_quantity);
            }
            Remove_Boxes(&(Store.db), Store.robots[self - 1].low_SKU, &(glb_time), &(event_id), self);
            Store.robots[self - 1].cur_box = Store.robots[self - 1].low_SKU;
            fprintf(f, "%*d %*d %*d     movebox2botREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col, 2, 0);
            Print_Channel(Store.robots[self - 1].col, f);
            fprintf(control_system_log, "%*d %*d %*d     movebox2bot       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;

            Store.robots[self - 1].has_box = 1;
            Store.robots[self - 1].goal_cell.id = (((Store.robots[self - 1].col / 50) + 1) * 12) + (4 - ((Store.robots[self - 1].col - 50 * (Store.robots[self - 1].col / 50)) / 10)) + 1;

            //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            Store.robots[self - 1].prev_box_type = Store.robots[self - 1].low_SKU;
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id += 1;
            //event_id += 1;
          }

        } else if (Store.robots[self - 1].has_box == 1 && Store.robots[self - 1].cur_cell.id == Store.robots[self - 1].goal_cell.id) {
          
          Store.robots[self - 1].goal_time = 8;
          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, Store.robots[self - 1].cur_cell.id + 1);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            fprintf(f, "%*d %*d %*d     startMotionREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
          }
          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {

            if (Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != -1 && Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, REVERSE, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }

            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;

            int row_to_add = 0;
            while (Store.conveyor[Store.robots[self - 1].col].boxes[row_to_add].SKU == -1) {
              row_to_add += 1;
            }
            row_to_add -= 1;
            
            int save = Store.robots[self - 1].row;
            Store.robots[self - 1].row = row_to_add;

            // Print_Channel(Store.robots[self - 1].col, f);
            Add_Box(&(Store.db), Store.robots[self - 1].low_SKU, self);
            fprintf(f, "%*d %*d %*d movebox2channelREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col, 2, 0);
            Print_Channel(Store.robots[self - 1].col, f);
            fprintf(control_system_log, "%*d %*d %*d movebox2channel       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
            //Print_Channel(Store.robots[self - 1].col, f);
            Store.robots[self - 1].low_SKU = -1;

            Store.robots[self - 1].row = save + 1;

            Store.robots[self - 1].has_box = -1;

            //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            Store.robots[self - 1].prev_box_type = 0;
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id += 1;
            //event_id += 1;
          }
        } else {
          
          int next_vert = next_vertex(Store.robots[self - 1].cur_cell.id, Store.robots[self - 1].goal_cell.id);

          if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && Store.direction_graph[next_vert] != -1) { // если оба стыка
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && (next_vert == 0 || next_vert == 43)) { // если стык и конечная
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 0) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 1) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 4;
          } else {
            Store.robots[self - 1].goal_time = 6;
          }

          if (Store.robots[self - 1].cur_time == 1) {
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            add_to_queue(self - 1, next_vert);
            del_from_queue(self - 1);
            if (Store.robots[self - 1].has_box == 1) {
              fprintf(f, "%*d %*d %*d     startMotionREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            } else {
              fprintf(f, "%*d %*d %*d     startMotionREVERSE       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4,Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4,Store.robots[self - 1].col % 10 + 1, 2, 0);
            }
            event_id += 1;
            control_id += 1;
          }

          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            if (Store.cells[next_vert].queue[0] != -1 && Store.cells[next_vert].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, TAKE_IN, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }
            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;

            if (Store.robots[self - 1].has_box == 1) {
              //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, Store.robots[self - 1].low_SKU, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              //event_id += 1;
              Store.robots[self - 1].prev_box_type = Store.robots[self - 1].low_SKU;
              Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
              Store.robots[self - 1].prev_tr_id = 0;
            } else {
              Store.robots[self - 1].prev_box_type = 0;
              Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
              Store.robots[self - 1].prev_tr_id = 0;
              //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              //event_id += 1;
            }
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            
            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id = next_vert;
          }
        }
        
        if (self == MAX_ROBOTS) {
          Send_Event(0, REVERSE, lp, &(lp->gid));
        } else {
          Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
        }
        break;
      case GO:
        if (cur_boxes >= 1190) {
          break;
        }

        if (Store.robots[self - 1].cur_cell.id == Store.robots[self - 1].goal_cell.id) {

          if (Store.robots[self - 1].cur_time == 1) {
            Store.robots[self - 1].goal_time = 6;
            add_to_queue(self - 1, Store.robots[self - 1].cur_cell.id + 1);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionGO       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            fprintf(f, "%*d %*d %*d     startMotionGO       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            event_id += 1;
            control_id += 1;
          }

          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {

            if (Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != -1 && Store.cells[Store.robots[self - 1].cur_cell.id + 1].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, GO, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }

            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;
            Store.robots[self - 1].cur_task = -1;
            //fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id + 1], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            Store.robots[self - 1].prev_box_type = 0;
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id += 1;
            //event_id += 1;
          }
        } else {
          int next_vert = next_vertex(Store.robots[self - 1].cur_cell.id, Store.robots[self - 1].goal_cell.id);
          
          if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && Store.direction_graph[next_vert] != -1) { // если оба стыка
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.direction_graph[Store.robots[self - 1].cur_cell.id] != -1 && (next_vert == 0 || next_vert == 43)) { // если стык и конечная
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 0) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 3;
          } else if (Store.robots[self - 1].cur_cell.id == 1) { // если в начале (паллета на загрузку)
            Store.robots[self - 1].goal_time = 4;
          } else {
            Store.robots[self - 1].goal_time = 6;
          }
          if (Store.robots[self - 1].cur_time == 1) {
            add_to_queue(self - 1, next_vert);
            del_from_queue(self - 1);
            if (glb_time > 1) {
              fprintf(f, "%*d %*d %*d    finishMotionGO       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].prev_vertex], 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.robots[self - 1].prev_box_type, 4, Store.robots[self - 1].prev_channel, 2, Store.robots[self - 1].prev_tr_id);
              event_id += 1;
            }
            if (Store.robots[self - 1].has_box == 1) {
              fprintf(f, "%*d %*d %*d     startMotionGO       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            } else {
              fprintf(f, "%*d %*d %*d     startMotionGO       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
              fprintf(control_system_log, "%*d %*d %*d     startMotion       %*s     %*s     %*d    %*d   %*d\n", 6, control_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            }
            control_id += 1;
            event_id += 1;
          }

          if (Store.robots[self - 1].cur_time >= Store.robots[self - 1].goal_time) {
            if (Store.cells[next_vert].queue[0] != -1 && Store.cells[next_vert].queue[0] != self - 1) {
              if (self == MAX_ROBOTS) {
                Send_Event(0, GO, lp, &(lp->gid));
              } else {
                Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
              }
              break;
            }
            Store.robots[self - 1].tmp_fl = 1;

            Store.robots[self - 1].cur_time = 0;

            // if (Store.robots[self - 1].has_box == 1) {
            //   Store.robots[self - 1].prev_box_type = 0;
            //   Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            //   Store.robots[self - 1].prev_tr_id = 0;
            //   // fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            //   // event_id += 1;
            // } else {
            //   Store.robots[self - 1].prev_box_type = 0;
            //   Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            //   Store.robots[self - 1].prev_tr_id = 0;
            //   // fprintf(f, "%*d %*d %*d    finishMotion       %*s     %*s     %*d    %*d   %*d\n", 6, event_id, 6, glb_time, 4, self, 4, Store.vertexes[Store.robots[self - 1].cur_cell.id], 4, Store.vertexes[next_vert], 4, 0, 4, Store.robots[self - 1].col % 10 + 1, 2, 0);
            //   // event_id += 1;
            // }
            Store.robots[self - 1].prev_box_type = 0;
            Store.robots[self - 1].prev_channel = Store.robots[self - 1].col % 10 + 1;
            Store.robots[self - 1].prev_tr_id = 0;
            Store.robots[self - 1].prev_vertex = Store.robots[self - 1].cur_cell.id;
            
            del_from_queue(self - 1);
            Store.robots[self - 1].cur_cell.id = next_vert;
          }
        }
      
        if (self == MAX_ROBOTS) {
          Send_Event(0, GO, lp, &(lp->gid));
        } else {
          Send_Event(self + 1, Store.messages[self].type, lp, &(lp->gid));
        }
        break;

      default:
        printf("\n%s\n", "No message");
        break;
    }
  }
}

//Reverse Event Handler
void model_event_reverse (state *s, tw_bf *bf, message *in_msg, tw_lp *lp) {
  return;
}

//report any final statistics for this LP
void model_final (state *s, tw_lp *lp) {
  if (lp->gid == 0) {
    write_csv("Store.csv", Store.db);
    fprintf(paleta, "%*d %*d %*s %*s", 6, rec_id, 6, glb_time, 18, "finishPalletize", 22, Store.cur_order);
    fprintf(paleta, "\n");
    rec_id++;
    fprintf(f, "%*d %*d      finishPalletize %s", 6, event_id, 6, glb_time, Store.cur_order);
  }
  return;
}


//Given an LP's GID (global ID)
//return the PE (aka node, MPI Rank)
tw_peid model_map(tw_lpid gid){
  return (tw_peid) gid / g_tw_nlp;
}





// Define LP types
//   these are the functions called by ROSS for each LP
//   multiple sets can be defined (for multiple LP types)
tw_lptype model_lps[] = {
  {
    (init_f) model_init,
    (pre_run_f) NULL,
    (event_f) model_event,
    (revent_f) model_event_reverse,
    (commit_f) NULL,
    (final_f) model_final,
    (map_f) model_map,
    sizeof(state)
  },
  { 0 },
};

//Define command line arguments default values
unsigned int setting_1 = 0;

//add your command line opts
const tw_optdef model_opts[] = {
	TWOPT_GROUP("ROSS Model"),
	TWOPT_UINT("setting_1", setting_1, "first setting for this model"),
	TWOPT_END(),
};

void displayModelSettings()
{
  if (g_tw_mynode == 0)
  {
    for (int i = 0; i < 30; i++)
      printf("*");
    
    printf("\n");
    printf("Model Configuration:\n");
    printf("\t nnodes: %i\n", tw_nnodes());
    printf("\t g_tw_nlp: %llu\n", g_tw_nlp);

    for (int i = 0; i < 30; i++)
      printf("*");
    
    printf("\n");
  }
}


//for doxygen
#define model_main main


int model_main (int argc, char* argv[]) {
  //bots_starting_positions = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/bots_starting_positions.csv", "r");
  bots_starting_positions = fopen("/home/sasha/Port/bots_starting_positions.csv", "r");

 // f = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/full_actions_log.txt", "w");
 // paleta = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/paletize_depaletize.txt", "w");
 // file = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/TEST1-SIMSIM/small_test.csv", "r");
//  f_dep = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/first_depalitization.txt", "w");
 // temp_txt = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/temp_txt.txt", "w");
 // csv_file = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/final_warehouse.csv", "w");
 // test = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/test.txt", "w");
 // robots_positions = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/robots_positions.csv", "w");
  //control_system_log = fopen("/Users/glebkruckov/Documents/Работа/Port/port-model/control_system_log.txt", "w");
  control_system_log = fopen("/home/sasha/Port/control_system_log.txt", "w");
  // const char *directory_path = "/Users/glebkruckov/Documents/Работа/Port/port-model/TEST3-SIMSIM";
  const char *directory_path = "/home/sasha/Port/TEST3-SIMSIM";
  struct dirent *entry;
  DIR *dp = opendir(directory_path);

  paleta = fopen("/home/sasha/Port/report.txt", "w");
  temp_txt = fopen("/home/sasha/Port/temp_txt.txt", "w");
  csv_file = fopen("/home/sasha/Port/csv_file.csv", "w");

  test = fopen("/home/sasha/Port/test.txt", "w");

  robots_positions = fopen("/home/sasha/Port/robots_positions.csv", "w");

  f_dep = fopen("/home/sasha/Port/first_depalitization.txt", "w");
  sqlite3_open("/home/sasha/Port/ross-sqlite.db", &Store.db);

  f = fopen("/home/sasha/Port/full_actions_log.txt", "w");
  file = fopen("/home/sasha/Port/TEST1-SIMSIM/small_test.csv", "r");


  while ((entry = readdir(dp))) {
    if (strstr(entry->d_name, ".csv") != NULL) {
      snprintf(Store.files[Store.cur_file], sizeof(Store.files[Store.cur_file]), "%s/%s", directory_path, entry->d_name);
      Store.cur_file += 1;
    }
  }
  Store.cur_file = 0;

  fprintf(paleta, " RecId   Time             Action              PalletID PalletTypeID\n");

 // sqlite3_open("/Users/glebkruckov/Documents/Работа/Port/port-model/ross-sqlite.db", &Store.db);
  fprintf(f, "EventID  Time BotID        Command StartPoint EndPoint  BoxType Channel TrID\n");
  fprintf(control_system_log, "EventID  Time BotID        Command StartPoint EndPoint  BoxType Channel TrID\n");


	InitROSS();
	int i, num_lps_per_pe;
  tw_opt_add(model_opts);
  tw_init(&argc, &argv);
  num_lps_per_pe = MAX_ROBOTS + 1;
  // g_tw_events_per_pe = 100000;
  tw_define_lps(num_lps_per_pe, sizeof(message));
  // displayModelSettings();
  g_tw_lp_typemap = &model_map;
  for (int i = 0; i < g_tw_nlp; ++i)
    tw_lp_settype(i, &model_lps[0]);

	// displayModelSettings();

	// Do some file I/O here? on a per-node (not per-LP) basis

	tw_run();

	tw_end();
  fprintf(f, "------------------------------------------------------------------------------------\n");
  fprintf(f, "------------------------------------------------------------------------------------\n");
  fprintf(f, "------------------------------------------------------------------------------------\n");

  // for (int i = 0; i < MAX_VERTEXES; ++i) {
  //   fprintf(test, "CELL %s. QUEUE", Store.vertexes[i]);
  //   for (int j = 0; j < MAX_ROBOTS; ++j) {
  //     fprintf(test, "%d ", Store.cells[i].queue[j]);
  //   }
  //   fprintf(test, "\n");
  // }

	return 0;
}
