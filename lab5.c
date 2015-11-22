#include "config.h"
#define use_uart_serial
#include "pt_cornell_1_2.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>

#define PAWN   1
#define BISHOP 2
#define KNIGHT 3
#define ROOK   4
#define QUEEN  5
#define KING   6
#define RESET  -1
#define IDLE   0
#define CALC   1

static double value;
char buffer[60];
static char cmd[2];
static struct pt pt_print, pt_key, pt_move, pt_input, pt_output, pt_DMA_output ;
int sys_time_seconds, state = -1;
static char signal = 1;
char init_x, init_y, end_x, end_y; //positions that will be selected by user via buttons

char current_player = 1;

typedef struct piece{
    char name;      //what piece it is
    char side;      //side = 1, it's white. -1 = black
    char xpos;      //ranges between a-h (a = 0, h = 7)
    char ypos;      //ranges between 1 and 8 (1 = 0, 8 = 7) 
}piece;

// our 8x8 board of pieces
piece *board[8][8] = {0};
piece *board1[8][8] = {0};
piece *board2[8][8] = {0};
piece *board3[8][8] = {0};
piece *board4[8][8] = {0};
piece *board5[8][8] = {0};
piece *board6[8][8] = {0};
piece *board7[8][8] = {0};
piece *board8[8][8] = {0};
piece *board9[8][8] = {0};
piece *board10[8][8] = {0};
char prev_board_counter = 0;

char avail[8][8] = {0};

void copy_two_boards(piece *board_src[8], piece *board_dest[8]){
	char i, j;
	for (j=0; j<8; j++)
		for (i=0; i<8; i++)
			board_dest[j][i] = board_src[j][i];
}

void copy_prev_board(){
	
	prev_board_counter++;
	piece (*board_src)[8];
	piece (*board_dest)[8];
	
	board_src = board;
	
	if (prev_board_counter == 1)
		board_dest = board1;
	else if (prev_board_counter == 2)
		board_dest = board2;
	else if (prev_board_counter == 3)
		board_dest = board3;
	else if (prev_board_counter == 4)
		board_dest = board4;
	else if (prev_board_counter == 5)
		board_dest = board5;
	else if (prev_board_counter == 6)
		board_dest = board6;
	else if (prev_board_counter == 7)
		board_dest = board7;
	else if (prev_board_counter == 8)
		board_dest = board8;
	else if (prev_board_counter == 9)
		board_dest = board9;
	else if (prev_board_counter == 10)
		board_dest = board10;
	else if (prev_board_counter == 11){ //reset counter back to 1
		prev_board_counter = 1;
		board_dest = board1;
	}
		
	copy_two_boards(board_src, board_dest);
}

void restore_prev_board(){
	
	if (prev_board_counter != 0){ //make sure we actually have a board to restore from
		
		piece (*board_src)[8];
		piece (*board_dest)[8];
	
		board_dest = board;
		
		if (prev_board_counter == 1)
			board_src = board1;
		else if (prev_board_counter == 2)
			board_src = board2;
		else if (prev_board_counter == 3)
			board_src = board3;
		else if (prev_board_counter == 4)
			board_src = board4;
		else if (prev_board_counter == 5)
			board_src = board5;
		else if (prev_board_counter == 6)
			board_src = board6;
		else if (prev_board_counter == 7)
			board_src = board7;
		else if (prev_board_counter == 8)
			board_src = board8;
		else if (prev_board_counter == 9)
			board_src = board9;
		else if (prev_board_counter == 10)
			board_src = board10;
		else if (prev_board_counter == 0){ //reset counter back to 1
			prev_board_counter = 10;
			board_src = board1;
		}
		
		prev_board_counter--;
		
		copy_two_boards(board_src, board_dest);
	}
}

void initialize_board(){
		
  piece *pawn;
  piece *rook;
  piece *knight;
  piece *bishop;
  piece *queen;
  piece *king;
	
	char side_loop = -1;

	//initialize white pawns
  char j, i;
	for (j = 1; j < 8; j+=5){
		side_loop = side_loop * -1;
		for (i = 0; i < 8; i++){
			pawn = (piece*)malloc(sizeof(piece));
			board[j][i] = pawn;
			pawn->name = PAWN;
			pawn->side = side_loop;
			pawn->xpos = i;
			pawn->ypos = j;
		}	
	}
	
	side_loop = -1;
	//initialize rooks
	for (j = 0; j < 8; j+=7){
		side_loop = side_loop * -1;
		for (i = 0; i < 8; i+=7){
			rook = (piece*)malloc(sizeof(piece));
			board[j][i] = rook;
			rook->name = ROOK;
			rook->side = side_loop;
			rook->xpos = i;
			rook->ypos = j;
		}
	}
	
	side_loop = -1;
	//initialize knights
	for (j = 0; j < 8; j+=7){
		side_loop = side_loop * -1;
		for (i = 1; i < 8; i+=5){
			knight = (piece*)malloc(sizeof(piece));
			board[j][i] = knight;
			knight->name = KNIGHT;
			knight->side = side_loop;
			knight->xpos = i;
			knight->ypos = j;
		}
	}
	
	side_loop = -1;
	
	//initialize bishop
	for (j = 0; j < 8; j+=7){
		side_loop = side_loop * -1;
		for (i = 2; i < 8; i+=3){
			bishop = (piece*)malloc(sizeof(piece));
			board[j][i] = bishop;
			bishop->name = BISHOP;
			bishop->side = side_loop;
			bishop->xpos = i;
			bishop->ypos = j;
		}
	}
	
	side_loop = -1;
	//initialize queen
	for (j = 0; j < 8; j+=7){
		side_loop = side_loop * -1;
		queen = (piece*)malloc(sizeof(piece));
		board[j][3] = queen;
		queen->name = QUEEN;
		queen->side = side_loop;
		queen->ypos = j;
		queen->xpos = 3;
	}
	
	side_loop = -1;
	
	//initialize king
	for (j = 0; j < 8; j+=7){
		side_loop = side_loop * -1;
		king = (piece*)malloc(sizeof(piece));
		board[j][4] = king;
		king->name = KING;
		king->side = side_loop;
		king->xpos = 4;
		king->ypos = j;
	}
	
}
void calc_pawn_moves(piece *pawn){	
	char pawn_y_sideff = pawn->ypos + (pawn->side * 2);
	char pawn_y_sidef = pawn->ypos + (pawn->side * 1);
	char pawn_x_sider = pawn->xpos + 1;
	char pawn_x_sidel = pawn->xpos - 1;
	avail[pawn->ypos][pawn->xpos] = 1;	
	//two squares up
	if ( ((pawn->ypos == 1 && pawn->side == 1) ||(pawn->ypos == 6 && pawn->side == -1))  && (board[pawn_y_sideff][pawn->xpos] == 0))//no piece should be there
		avail[pawn_y_sideff][pawn->xpos] = 1;
	
	//diag right
	if (((pawn_x_sider)<8) && (board[pawn_y_sidef][pawn_x_sider] != 0) && (board[pawn_y_sidef][pawn_x_sider]->side != pawn->side)) //see if piece is there
		avail[pawn_y_sidef][pawn_x_sider] = 1;

	//diag left
	if (((pawn_x_sidel)>-1) && (board[pawn_y_sidef][pawn_x_sidel] != 0) && (board[pawn_y_sidef][pawn_x_sidel]->side != pawn->side)) //see if enemy piece is there
		avail[pawn_y_sidef][pawn_x_sidel] = 1;
	
	//one square up
	if (board[pawn_y_sidef][pawn->xpos] == 0) //no piece should be in front of pawn
		avail[pawn_y_sidef][pawn->xpos] = 1;	
}
void calc_bish_moves(piece *bishop){
	
	char bish_x = bishop->xpos;
	char bish_y = bishop->ypos;	
    char i;
    
    avail[bish_y][bish_x] = 1;
	for (i = 1; bish_x+i<8; i++){ //go diag up right
		if ((bish_y-i)>-1){
			if (board[bish_y-i][bish_x+i] == 0)
				avail[bish_y-i][bish_x+i] = 1;
			else if (board[bish_y-i][bish_x+i]->side != bishop->side){
				avail[bish_y-i][bish_x+i] = 1;
				break;
			}
			else break;
		}
		else break;//this move is invalid. so are rest in line
	}
	
	for (i = 1; bish_x+i<8; i++){ //go diag down right
		if ((bish_y+i)<8){
			if (board[bish_y+i][bish_x+i] == 0)
				avail[bish_y+i][bish_x+i] = 1;
			else if (board[bish_y+i][bish_x+i]->side != bishop->side){
				avail[bish_y+i][bish_x+i] = 1;
				break;
			}
			else break;
		}
		else break;//this move is invalid. so are rest in line
	}
	
	for (i = 1; bish_x-i>-1; i++){ //go diag down left
		if ((bish_y+i)<8){
			if (board[bish_y+i][bish_x-i] == 0)
				avail[bish_y+i][bish_x-i] = 1;
			else if (board[bish_y+i][bish_x-i]->side != bishop->side){
				avail[bish_y+i][bish_x-i] = 1;
				break;
			}
			else break;
		}
		else break;//this move is invalid. so are rest in line
	}
	
	for (i = 1; bish_x-i>-1; i++){ //go diag up left
		if ((bish_y-i)>-1){
			if (board[bish_y-i][bish_x-i] == 0)
				avail[bish_y-i][bish_x-i] = 1;
			else if (board[bish_y-i][bish_x-i]->side != bishop->side){
				avail[bish_y-i][bish_x-i] = 1;
				break;
			}
			else break;
		}			
		else break;//this move is invalid. so are rest in line
	}
}
void calc_rook_moves(piece *rook){
	
	char rook_x = rook->xpos;
	char rook_y = rook->ypos;
    char i;  
	
    avail[rook_y][rook_x] = 1;
	for (i = 1; rook_y-i>-1; i++){ //go up
		if (board[rook_y-i][rook_x] == 0) //make sure no piece there or it's enemy piece
			avail[rook_y-i][rook_x] = 1;
		else if (board[rook_y-i][rook_x]->side != rook->side){
			avail[rook_y-i][rook_x] = 1;
			break;
		}
		else break;//this move is invalid. so are rest in line
	}
	
	for (i = 1; rook_x+i<8; i++){ //go right
		if (board[rook_y][rook_x+i] == 0)  //make sure no piece there or it's enemy piece
			avail[rook_y][rook_x+i] = 1;
		else if (board[rook_y][rook_x+i]->side != rook->side){
			avail[rook_y][rook_x+i] = 1;
			break;
		}
		else break;//this move is invalid. so are rest in line
	}
	
	for (i = 1; rook_y+i<8; i++){ //go down
		if (board[rook_y+i][rook_x] == 0) //make sure no piece there or it's enemy piece
			avail[rook_y+i][rook_x] = 1;
		else if (board[rook_y+i][rook_x]->side != rook->side){
			avail[rook_y+i][rook_x] = 1;
			break;
		}
		else break;//this move is invalid. so are rest in line
	}
	
	for (i = 1; rook_x-i>-1; i++){ //go left
		if (board[rook_y][rook_x-i] == 0) //make sure no piece there or it's enemy piece
			avail[rook_y][rook_x-i] = 1;
		else if (board[rook_y][rook_x-i]->side != rook->side){
			avail[rook_y][rook_x-i] = 1;
			break;
		}
		else break;//this move is invalid. so are rest in line
	}
}
void calc_queen_moves(piece *queen){
	calc_rook_moves(queen);
	calc_bish_moves(queen);
}
void calc_king_moves(piece *king){
	
	char king_x = king->xpos;
	char king_y = king->ypos;
    
    avail[king_y][king_x] = 1;
	//up
	if (((king_y-1) > -1) && ((board[king_y-1][king_x] == 0) || (board[king_y-1][king_x]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y-1][king_x] = 1;
	
	//up right
	if (((king_y-1)>-1) && ((king_x+1)<8) && ((board[king_y-1][king_x+1] == 0) || (board[king_y-1][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y-1][king_x+1] = 1;
	
	//right
	if (((king_x+1)<8) && ((board[king_y][king_x+1] == 0) || (board[king_y][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y][king_x+1] = 1;
	
	//down right
	if (((king_y+1)<8) && ((king_x+1)<8) && ((board[king_y+1][king_x+1] == 0) || (board[king_y+1][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y+1][king_x+1] = 1;
	
	//down
	if ((king_y+1<8) && ((board[king_y+1][king_x] == 0) || (board[king_y+1][king_x]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y+1][king_x] = 1;
	
	//down left
	if (((king_y+1)<8) && ((king_x-1)>-1) && ((board[king_y+1][king_x-1] == 0) || (board[king_y+1][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y+1][king_x-1] = 1;
	
	//left
	if (((king_x-1)>-1) && ((board[king_y][king_x-1] == 0) || (board[king_y][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y][king_x-1] = 1;
	
	//up left
	if (((king_y-1)>-1) && ((king_x-1)>-1) && ((board[king_y-1][king_x-1] == 0) || (board[king_y-1][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		avail[king_y-1][king_x-1] = 1;
}
void calc_knight_moves(piece *knight){
	
	char knight_x = knight->xpos;
	char knight_y = knight->ypos;
	
    avail[knight_y][knight_x] = 1;
	//big up right
	if (((knight_y-2) > -1) && ((knight_x+1)<8) && ((board[knight_y-2][knight_x+1] == 0) || (board[knight_y-2][knight_x+1]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y-2][knight_x+1] = 1;
		
	//small up right
	if (((knight_y-1) > -1) && ((knight_x+2)<8) && ((board[knight_y-1][knight_x+2] == 0) || (board[knight_y-1][knight_x+2]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y-1][knight_x+2] = 1;
	
    //big up left
	if (((knight_y-2) > -1) && ((knight_x-1)>-1) && ((board[knight_y-2][knight_x-1] == 0) || (board[knight_y-2][knight_x-1]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y-2][knight_x-1] = 1;	
    
	//small up left
	if (((knight_y-1) > -1) && ((knight_x-2)>-1) && ((board[knight_y-1][knight_x-2] == 0) || (board[knight_y-1][knight_x-2]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y-1][knight_x-2] = 1;
    
    //big down right
	if (((knight_y+2) < 8) && ((knight_x+1)<8) && ((board[knight_y+2][knight_x+1] == 0) || (board[knight_y+2][knight_x+1]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y+2][knight_x+1] = 1;
    
	//small down right
	if (((knight_y+1) < 8) && ((knight_x+2)<8) && ((board[knight_y+1][knight_x+2] == 0) || (board[knight_y+1][knight_x+2]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y+1][knight_x+2] = 1;
	
	//big down left
	if (((knight_y+2) < 8) && ((knight_x-1)>-1) && ((board[knight_y+2][knight_x-1] == 0) || (board[knight_y+2][knight_x-1]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y+2][knight_x-1] = 1;
		
	//small down left
	if (((knight_y+1) < 8) && ((knight_x-2)>-1) && ((board[knight_y+1][knight_x-2] == 0) || (board[knight_y+1][knight_x-2]->side != knight->side))) //make sure no piece there or it's enemy piece
		avail[knight_y+1][knight_x-2] = 1;
			
	
}
void reset_array(){
	char i, j;	
	for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
			avail[i][j] = 0;
}
void calc_piece_moves(){
	if (current_player == board[init_y][init_x]->side){ //make sure piece belongs to current player's side. otherwise, do nothing.
		char piece_name = board[init_y][init_x]->name;
		reset_array(); //reset avail array for each piece
		
		tft_fillRect(0, 30, 240, 20, ILI9340_BLACK);// x,y,w,h,color
		tft_setCursor(0, 30);
		tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
		
		switch(piece_name)  {
			case 1:
				tft_writeString("Pawn");
				break;
			case 2:
				tft_writeString("Bishop");
				break;
			case 3:
				tft_writeString("Knight");
				break;
			case 4:
				tft_writeString("Rook");
				break;
			case 5:
				tft_writeString("Queen");
				break;
			case 6:
				tft_writeString("King");
				break;
			default:
				tft_writeString("Invalid Piece");          
		}

		if (piece_name == PAWN)
			calc_pawn_moves(board[init_y][init_x]);
		else if (piece_name == BISHOP)
			calc_bish_moves(board[init_y][init_x]);
		else if (piece_name == KNIGHT)
			calc_knight_moves(board[init_y][init_x]);
		else if (piece_name == ROOK)
			calc_rook_moves(board[init_y][init_x]);
		else if (piece_name == QUEEN)
			calc_queen_moves(board[init_y][init_x]);
		else if (piece_name == KING)
			calc_king_moves(board[init_y][init_x]);	
	}
}
void move_piece_pawn_check(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
	//now move piece to new position and set prev to 0 since it's not there anymore
	board[new_pos_y][new_pos_x] = 0;
	board[new_pos_y][new_pos_x] = board[pos_y][pos_x];
	board[pos_y][pos_x] = 0;
	
	if (board[new_pos_y][new_pos_x]->name == PAWN){ //see if new move made is pawn to end
		if ((board[new_pos_y][new_pos_x]->side == 1) && (new_pos_y == 0)){ //see if white pawn hit end
			piece *new_queen = (piece*)malloc(sizeof(piece));
			new_queen->name = QUEEN;
			new_queen->side = board[new_pos_y][new_pos_x]->side;
			new_queen->ypos = new_pos_y;
			new_queen->xpos = new_pos_x;
			board[new_pos_y][new_pos_x] = new_queen;
		}
		else if ((board[new_pos_y][new_pos_x]->side == -1) && (new_pos_y == 7)){ //see if black pawn hit end
			piece *new_queen = (piece*)malloc(sizeof(piece));
			new_queen->name = QUEEN;
			new_queen->side = board[new_pos_y][new_pos_x]->side;
			new_queen->ypos = new_pos_y;
			new_queen->xpos = new_pos_x;
			board[new_pos_y][new_pos_x] = new_queen;
		}
	}
}
void move_piece(){
	
	piece *piece1 = board[init_y][init_x];
	if (avail[end_y][end_x]){
        piece1->xpos = end_x;
        piece1->ypos = end_y;
		move_piece_pawn_check(init_x, init_y, piece1->xpos, piece1->ypos);
		//move has been made. now other player's turn
		current_player = current_player * -1;
		copy_prev_board();
	}
}
// take keyboard commands. for testing purpose
static PT_THREAD (protothread_keyboard(struct pt *pt)){
    PT_BEGIN(pt);
      while(1) {         
        // send the prompt via DMA to serial
        sprintf(PT_send_buffer,"cmd>");
        // by spawning a print thread
        PT_SPAWN(pt, &pt_DMA_output, PT_DMA_PutSerialBuffer(&pt_DMA_output) );
        //spawn a thread to handle terminal input
        // the input thread waits for input
        // -- BUT does NOT block other threads
        // string is returned in "PT_term_buffer"
        PT_SPAWN(pt, &pt_input, PT_GetSerialBuffer(&pt_input) );
        // returns when the thread dies
        // in this case, when <enter> is pushed
        // now parse the string
        sscanf(PT_term_buffer, "%s", cmd);
        if(cmd[0] == 'z') 
            printf("%i%1i\t %i%1i\n\r", init_x, init_y, end_x, end_y);      
        else if(cmd[0] >= 97 && cmd[0] <= 104 && cmd[1] >= 48 && cmd[1] <= 56)   {
            if(board[cmd[1]-49][cmd[0]-97] == NULL && state == IDLE);
            else if(state == IDLE)   {
                init_x = cmd[0] - 97;
                init_y = cmd[1] - 49;
                state++;
                signal = 1;
            }
            else if (init_x == cmd[0]-97 && init_y == cmd[1] -49)   {
                state = -1;
                signal = 1;
            }
            else if (avail[cmd[1]-49][cmd[0]-97] == NULL && state == CALC);
            else if (state == CALC) {
                end_x = cmd[0] - 97;
                end_y = cmd[1] - 49;
                state = 0;
                signal = 1;
            }
        }
    } // END WHILE(1)
    PT_END(pt);
}
// === TFT Print Thread =================================================
static PT_THREAD (protothread_print(struct pt *pt)){
    PT_BEGIN(pt);
    int i, j;
    tft_setCursor(0, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    tft_writeString("Refresh every 2 secs\n");
    
    //Print the chess board outer parameter A-H; 1-8
    tft_setTextColor(ILI9340_GREEN);  tft_setTextSize(2);
    for(i = 0; i < 8; i++)  {
        tft_setCursor(50+20*i, 60);
        sprintf(buffer, "%c", 65+i);
        tft_writeString(buffer);
        tft_setCursor(50+20*i, 240);
        sprintf(buffer, "%c", 65+i);
        tft_writeString(buffer);
        
        tft_setCursor(30, 220-20*i);
        sprintf(buffer, "%i", i+1);
        tft_writeString(buffer);
        tft_setCursor(210, 220-20*i);
        sprintf(buffer, "%i", i+1);
        tft_writeString(buffer);
    }
        //updates the chess board each iteration
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(2000) ;
        sys_time_seconds++ ;
        // draw sys_time
        tft_fillRect(0, 10, 50, 20, ILI9340_BLACK);// x,y,w,h,color
        tft_fillRect(50, 80, 160, 160, ILI9340_BLACK);// x,y,w,h,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer);
        for(j = 0; j < 8; j++ ) {
            for(i = 0; i < 8; i++)  {
                tft_setCursor(50+20*j, 220-20*i);
                tft_setTextSize(2);                  
                if(board[i][j] == NULL) {                       
                    tft_setTextColor(ILI9340_BLUE); 
                    tft_writeString("-");
                }
                else    {
                    if(board[i][j]->side == 1)
                        tft_setTextColor(ILI9340_WHITE);
                    else
                        tft_setTextColor(ILI9340_RED);
                    if (board[i][j]->name == PAWN) 
                        tft_writeString("P");
                    else if (board[i][j]->name == ROOK) 
                        tft_writeString("R");
                    else if (board[i][j]->name == KNIGHT) 
                        tft_writeString("H");
                    else if (board[i][j]->name == BISHOP) 
                        tft_writeString("B");
                    else if (board[i][j]->name == QUEEN) 
                        tft_writeString("Q");
                    else if (board[i][j]->name == KING) 
                        tft_writeString("K");
                }
                if(state == CALC && avail[i][j] == 1)   {    
                    tft_setCursor(53+20*j, 220-20*i);                                       
                    tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1);
                    tft_writeString("*");
                }
            }
        }
    } // END WHILE(1)
  PT_END(pt);
} // timer thread
// === Calculate Move and Make Move Thread =============================================
static PT_THREAD (protothread_move(struct pt *pt)){
    PT_BEGIN(pt);
        while(1) {       
          // if button or command is entered for both start and end, make the move
            PT_YIELD_UNTIL(pt, signal);
            if(state == RESET)  {
                tft_fillRect(50, 10, 50, 20, ILI9340_BLACK);// x,y,w,h,color
                tft_setCursor(50, 10);
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
                tft_writeString("IDLE");
                tft_fillRect(0, 30, 70, 20, ILI9340_BLACK);// x,y,w,h,color
                state++;
            }
            else if(state == IDLE)   {
                tft_fillRect(50, 10, 50, 20, ILI9340_BLACK);// x,y,w,h,color
                tft_setCursor(50, 10);
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
                tft_writeString("IDLE");
                tft_fillRect(0, 30, 70, 20, ILI9340_BLACK);// x,y,w,h,color
                move_piece();
            }
            else if(state == CALC)  {
                tft_fillRect(50, 10, 50, 20, ILI9340_BLACK);// x,y,w,h,color
                tft_setCursor(50, 10);
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
                tft_writeString("CALC");
                calc_piece_moves();
            }
            signal = 0;
        } // END WHILE(1)
	PT_END(pt);
}
// === Main  ======================================================
void main(void) {
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_print);
  PT_INIT(&pt_key);
  PT_INIT(&pt_move);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  
  initialize_board();

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_keyboard(&pt_key));
      PT_SCHEDULE(protothread_print(&pt_print));
      PT_SCHEDULE(protothread_move(&pt_move));
  }
 } // main
// === end  ======================================================
