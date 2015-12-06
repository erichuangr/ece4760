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
static char cmd[2], signal = 1;
static struct pt pt_end, pt_out, pt_key, pt_time, pt_move, pt_input, pt_output, pt_DMA_output ;
static struct pt pt_blink;
int white_time_seconds = 300, black_time_seconds = 300, state = -1;
char buffer[60], init_x, init_y, end_x, end_y, current_player = 1; //positions that will be selected by user via buttons
char white_king_taken = 0;
char black_king_taken = 0;

typedef struct piece{
    char name;      //what piece it is
    char side;      //side = 1, it's white. -1 = black
    char xpos;      //ranges between a-h (a = 0, h = 7)
    char ypos;      //ranges between 1 and 8 (1 = 0, 8 = 7) 
	char moved;     //1 = has moved this game
}piece;

// our 8x8 board of pieces
piece *board[8][8] = {0};
char avail[8][8] = {0};

//return 1 if white wins, -1 if black wins, 0 if still playing
char game_over(){
    if((white_time_seconds <= 0) || white_king_taken) 
        return -1;
    else if((black_time_seconds <= 0) || black_king_taken)
        return 1;
    else
        return 0;
    //add in logic if king is taken
}
char can_king_castle(piece *king){
	
	char side_row;
	if (king->side == 1) //white king
		side_row = 0;
	else
		side_row = 7;
	
	if (!(king->moved))
		if (board[side_row][7]->moved == 0) //if rook king side hasn't moved
			if ((board[side_row][5] == 0) && (board[side_row][6] == 0)) //if no pieces b/w rook and king
				return 1;	
	return 0;
}
char can_queen_castle(piece *king){
	
	char side_row;
	if (king->side == 1) //white king
		side_row = 0;
	else
		side_row = 7;
	
	if (king->moved == 0)
		if (board[side_row][0]->moved == 0)//if rook king side hasn't moved
			if ((board[side_row][1] == NULL) && (board[side_row][2] == NULL) && (board[side_row][3] == NULL)) //if no pieces b/w rook and king
				return 1;	
	return 0;
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
			rook->moved = 0;
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
		king->moved = 0;
	}
	
	/*white_king = board[7][4];
	black_king = board[0][4];
	
	white_queen = board[7][3];
	black_queen = board[0][3];
	
	white_bish1 = board[7][2];
	white_bish2 = board[7][5];
	black_bish1 = board[0][2];
	black_bish2 = board[0][5];
	
	white_rook1 = board[7][0];
	white_rook2 = board[7][7];
	black_rook1 = board[0][0];
	black_rook2 = board[0][7];*/
}
void calc_pawn_moves  (piece *pawn){	
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
void calc_bish_moves  (piece *bishop){
	
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
void calc_rook_moves  (piece *rook){
	
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
void calc_queen_moves (piece *queen){
	calc_rook_moves(queen);
	calc_bish_moves(queen);
}
void calc_king_moves  (piece *king){
	
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
		
	if (can_king_castle(king)) //can we castle king side
		avail[king_y][king_x+2] = 1;
	
	if (can_queen_castle(king)) //can we castle queen side
		avail[king_y][king_x-2] = 1;
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
    
    char piece_name = board[init_y][init_x]->name;	
    reset_array(); //reset avail array for each piece

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
void move_piece_pawn_check(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
    //check if we're capturing king
    if (board[new_pos_y][new_pos_x]!=NULL && board[new_pos_y][new_pos_x]->name == KING){ //king is going to be captured
        if (current_player == 1) //black king is taken
            black_king_taken = 1;
        else
            white_king_taken = 1;
    }
    
	//now move piece to new position and set prev to 0 since it's not there anymore
	board[new_pos_y][new_pos_x] = board[pos_y][pos_x];
	board[pos_y][pos_x] = 0;
    
	if (board[new_pos_y][new_pos_x]->name == PAWN){ //see if new move made is pawn to end
		if ((board[new_pos_y][new_pos_x]->side == 1) && (new_pos_y == 7)) //see if white pawn hit end
			board[new_pos_y][new_pos_x]->name = QUEEN;
		else if ((board[new_pos_y][new_pos_x]->side == -1) && (new_pos_y == 0)) //see if black pawn hit end
			board[new_pos_y][new_pos_x]->name = QUEEN;
	}	
	else if (board[new_pos_y][new_pos_x]->name == KING){ //check for castling
		if ((new_pos_y == pos_y) && (new_pos_x == (pos_x+2))){ //check if king side castle
			board[new_pos_y][new_pos_x-1] = board[new_pos_y][7]; //move rook with castle
			board[new_pos_y][7] = 0;
		}
		else if ((new_pos_y == pos_y) && (new_pos_x == (pos_x-2))){ //check if queen castle move
			board[new_pos_y][new_pos_x+1] = board[new_pos_y][0]; //move rook with castle
			board[new_pos_y][0] = 0;
		}
	}	
	
	if ((board[new_pos_y][new_pos_x]->name == KING) || (board[new_pos_y][new_pos_x]->name == ROOK))
		board[new_pos_y][new_pos_x]->moved = 1;
}
void move_piece(){
	
	piece *piece1 = board[init_y][init_x];
	if (avail[end_y][end_x]){
        piece1->xpos = end_x;
        piece1->ypos = end_y;
		move_piece_pawn_check(init_x, init_y, piece1->xpos, piece1->ypos);
		current_player = current_player * -1;       //move has been made. now other player's turn
	}
}

static PT_THREAD(protothread_endgame(struct pt *pt)){
    PT_BEGIN(pt); 
    PT_WAIT_UNTIL(pt, game_over() != 0);
    tft_fillRect(0, 0, 240, 320, ILI9340_BLACK);// x,y,w,h,color
    tft_setTextSize(3);
    tft_setCursor(30, 140);
    char game = game_over();
    if(game == 1)    {
        tft_setTextColor(ILI9340_WHITE);
        tft_writeString("White Wins!");
    }
    else    {
        tft_setTextColor(ILI9340_RED);
        tft_writeString("Black Wins!");
    }  
    while(1);
    PT_END(pt);
}
// === TFT Timer Thread =================================================
static PT_THREAD (protothread_timer(struct pt *pt)){
    PT_BEGIN(pt);
    tft_setCursor(0, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    tft_writeString("Time Left\n");    
    while(1) {         
        PT_YIELD_TIME_msec(1000);  // yield time 1 second
        if(current_player == 1 && white_time_seconds > 0)
            white_time_seconds--;
        else if(current_player == -1 && black_time_seconds > 0)
            black_time_seconds--;
        
        tft_fillRect(0, 10, 100, 20, ILI9340_BLACK);// x,y,w,h,color
        tft_setTextSize(2);
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_WHITE);      
        sprintf(buffer,"%d", white_time_seconds);
        tft_writeString(buffer);
        tft_setCursor(50, 10);
        tft_setTextColor(ILI9340_RED);
        sprintf(buffer,"%d", black_time_seconds);
        tft_writeString(buffer);
    } // END WHILE(1)
    PT_END(pt);
}
// === TFT Input Thread =================================================
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
            if((board[cmd[1]-49][cmd[0]-97] == NULL || (board[cmd[1]-49][cmd[0]-97]->side != current_player)) && state == IDLE);
           // if(board[cmd[1]-49][cmd[0]-97] == NULL && state == IDLE);
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
// === TFT Output Thread =================================================
static PT_THREAD (protothread_output(struct pt *pt)){
    PT_BEGIN(pt);
    int i, j;
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
        PT_YIELD_TIME_msec(1000);
        tft_fillRect(50, 80, 160, 160, ILI9340_BLACK);// x,y,w,h,color
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
                //prints out * on the pieces able to be selected depending on the players turn
                if(state == IDLE && board[i][j] != NULL && board[i][j]->side == current_player)    {
                    tft_setCursor(53+20*j, 220-20*i);                                       
                    tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1);
                    tft_writeString("*");
                }
                //print out * on the places able to move with that piece
                else if(state == CALC && avail[i][j] == 1)   {    
                    tft_setCursor(53+20*j, 220-20*i);                                       
                    tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1);
                    tft_writeString("*");
                }
            }
        }
        //prints out turns (for debugging purposes)
        tft_fillRect(0, 30, 90, 20, ILI9340_BLACK);// x,y,w,h,color
		tft_setCursor(0, 30);
		tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        if(current_player == 1)
            tft_writeString("White");
        else
            tft_writeString("Red");
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
                tft_fillRect(90, 30, 50, 20, ILI9340_BLACK);// x,y,w,h,color
                tft_setCursor(90, 30);
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
                tft_writeString("IDLE");
                tft_fillRect(0, 30, 70, 20, ILI9340_BLACK);// x,y,w,h,color
                state++;
            }
            else if(state == IDLE)   {
                tft_fillRect(90, 30, 50, 20, ILI9340_BLACK);// x,y,w,h,color
                tft_setCursor(90, 30);
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
                tft_writeString("IDLE");
                tft_fillRect(0, 30, 70, 20, ILI9340_BLACK);// x,y,w,h,color
                move_piece();
            }
            else if(state == CALC)  {
                tft_fillRect(90, 30, 50, 20, ILI9340_BLACK);// x,y,w,h,color
                tft_setCursor(90, 30);
                tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
                tft_writeString("CALC");
                calc_piece_moves();
            }
            signal = 0;
        } // END WHILE(1)
	PT_END(pt);
}
// === Blink LED =============================================
/*
static int xc = 225, yc = 15;
static PT_THREAD (protothread_blink(struct pt *pt)){
    char wait = 1;
    PT_BEGIN(pt);
        while(1) {       
          mPORTASetBits(BIT_3);
          mPORTBClearBits(BIT_4);
           
          //light 4
          mPORTBSetBits(BIT_8);
          mPORTBClearBits(BIT_6 | BIT_7);
		  PT_YIELD_TIME_msec(wait);
          
          //light 6
          mPORTBSetBits(BIT_8 | BIT_7);
          mPORTBClearBits(BIT_6);
		  PT_YIELD_TIME_msec(wait);
         
          //light 0
          //PORTBSetBits(BIT_6);
          mPORTBClearBits(BIT_6 | BIT_7 | BIT_8);
		  PT_YIELD_TIME_msec(wait);
          
          //light 2
          mPORTBSetBits(BIT_7);
          mPORTBClearBits(BIT_6 | BIT_8);
		  PT_YIELD_TIME_msec(wait);
          
          //light 1
          mPORTBSetBits(BIT_6);
          mPORTBClearBits(BIT_8 | BIT_7);
		  PT_YIELD_TIME_msec(wait);
          
          //light 7
          mPORTBSetBits(BIT_8 | BIT_7 | BIT_6);
          //mPORTBClearBits(BIT_6);
		  PT_YIELD_TIME_msec(wait);
         
          //light 3
          mPORTBSetBits(BIT_7 | BIT_6);
          mPORTBClearBits(BIT_8);
		  PT_YIELD_TIME_msec(wait);
          
          //light 5
          mPORTBSetBits(BIT_6 | BIT_8);
          mPORTBClearBits(BIT_7);
		  PT_YIELD_TIME_msec(wait);
           
        } // END WHILE(1)
	PT_END(pt);
}
 * */
// === Main  ======================================================
void main(void) {
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();
  /*
  // set pins for lights
  mPORTASetBits(BIT_3);
  mPORTASetPinsDigitalOut(BIT_3);
  mPORTBSetBits(BIT_4 | BIT_6 | BIT_7 | BIT_8);
  mPORTBSetPinsDigitalOut(BIT_4 | BIT_6 | BIT_7 | BIT_8);
*/
  // init the threads
  PT_INIT(&pt_time);
  PT_INIT(&pt_out);
  PT_INIT(&pt_key);
  PT_INIT(&pt_move);
  PT_INIT(&pt_end);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  
  initialize_board();

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_time));
      PT_SCHEDULE(protothread_keyboard(&pt_key));
      PT_SCHEDULE(protothread_output(&pt_out));
      PT_SCHEDULE(protothread_move(&pt_move));
      PT_SCHEDULE(protothread_endgame(&pt_end));
	//  PT_SCHEDULE(protothread_blink(&pt_blink));
  }
 } // main
// === end  ======================================================
