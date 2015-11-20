#include "config.h"
#define use_uart_serial
#include "pt_cornell_1_2.h"
#include "tft_master.h"
#include "tft_gfx.h"
#include <stdlib.h>

#define PAWN   0
#define BISHOP 1
#define KNIGHT 2
#define ROOK   3
#define QUEEN  4
#define KING   5
#define IDLE   0
#define CALC   1
#define MOVE   2
#define D      3
#define E      4
#define F      5
#define G      6
#define H      7

static double value;
char buffer[60];
static char cmd[2];
static struct pt pt_timer, pt_key, pt_move, pt_input, pt_output, pt_DMA_output ;
int sys_time_seconds, state;
char init_x, init_y, end_x, end_y;

char current_player = 1;

typedef struct piece{
	
	char name;   //what piece it is
	char start;  //0 means moved this game
	char alive;  //alive = 1, it's alive. 0 = dead
	char side;   //side = 1, it's white. -1 = black
    char xpos;   //ranges between a-h (a = 0, h = 7)
    char ypos;   //ranges between 1 and 8 (1 = 0, 8 = 7)
    
	char pawn_avail[4];
    char bishop_avail[28];
    char knight_avail[8];
    char rook_avail[28];
    char queen_avail[56];
    char king_avail[8];
}piece;

// our 8x8 board of pieces
piece *board[8][8] = {0};
piece *captured[29] = {0}; //captured elements
int captured_index = 0;

piece *selected;

piece *pawn;
piece *rook;
piece *knight;
piece *bishop;
piece *queen;
piece *king;
char side_loop = -1;

void initialize_board(){
		
	//initialize white pawns
    int j, i;
	for (j = 1; j < 8; j+=5){
		side_loop = side_loop * -1;
		for (i = 0; i < 8; i++){
			pawn = (piece*)malloc(sizeof(piece));
			board[j][i] = pawn;
			pawn->name = PAWN;
			pawn->start = 1;
			pawn->alive = 1;
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
			rook->start = 1;
			rook->alive = 1;
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
			knight->start = 1;
			knight->alive = 1;
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
			bishop->start = 1;
			bishop->alive = 1;
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
		queen->start = 1;
		queen->alive = 1;
		queen->side = side_loop;
		queen->ypos = 3;
		queen->xpos = j;
	}
	
	side_loop = -1;
	
	//initialize king
	for (j = 0; j < 8; j+=7){
		side_loop = side_loop * -1;
		king = (piece*)malloc(sizeof(piece));
		board[j][4] = king;
		king->name = KING;
		king->start = 1;
		king->alive = 1;
		king->side = side_loop;
		king->xpos = 4;
		king->ypos = j;
	}
	
}
void calc_pawn_moves(piece *pawn){
	
	char pawn_x = pawn->xpos;
	char pawn_y = pawn->ypos;
	
	char pawn_y_sideff = pawn->ypos + (pawn->side * 2);
	char pawn_y_sidef = pawn->ypos + (pawn->side * 1);
	char pawn_x_sider = pawn->xpos + 1;
	char pawn_x_sidel = pawn->xpos - 1;
		
	//two squares up
	if (((pawn->start) && (board[pawn_y_sideff][pawn_x] == 0))) //no piece should be there
		pawn->pawn_avail[0] = 1;
	else
		pawn->pawn_avail[0] = 0;
	
	//diag right
	if (((pawn_x_sider)<8) && (board[pawn_y_sidef][pawn_x_sider] != 0) && (board[pawn_y_sidef][pawn_x_sider]->side != pawn->side)) //see if piece is there
		pawn->pawn_avail[2] = 1;
	else
		pawn->pawn_avail[2] = 0;

	//diag left
	if (((pawn_x_sidel)>-1) && (board[pawn_y_sidef][pawn_x_sidel] != 0) && (board[pawn_y_sidef][pawn_x_sidel]->side != pawn->side)) //see if enemy piece is there
		pawn->pawn_avail[3] = 1;
	else
		pawn->pawn_avail[3] = 0;
	
	//one square up
	if (board[pawn_y_sidef][pawn_x_sidel] == 0) //no piece should be in front of pawn
		pawn->pawn_avail[1] = 1;
	else
		pawn->pawn_avail[1] = 0;
	
}
void calc_bish_moves(piece *bishop){
	
	char bish_x = bishop->xpos;
	char bish_y = bishop->ypos;
	
    int i, j;
    
	for (i = 1; bish_x+i<8; i++){ //go diag up right
		if (((bish_y-i)>-1) && ((board[bish_y-i][bish_x+i] == 0) || (board[bish_y-i][bish_x+i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->bishop_avail[i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->bishop_avail[i-1] = 0;
			for (j = 1; i-1+j < 7; j++)
				bishop->bishop_avail[i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; bish_x+i<8; i++){ //go diag down right
		if (((bish_y+i)<8) && ((board[bish_y+i][bish_x+i] == 0) || (board[bish_y+i][bish_x+i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->bishop_avail[7+i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->bishop_avail[7+i-1] = 0;
			for (j = 1; 7+i-1+j < 14; j++)
				bishop->bishop_avail[7+i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; bish_x-i>-1; i++){ //go diag down left
		if (((bish_y+i)<8) && ((board[bish_y+i][bish_x-i] == 0) || (board[bish_y+i][bish_x-i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->bishop_avail[14+i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->bishop_avail[14+i-1] = 0;
			for (j = 1; 14+i-1+j < 21; j++)
				bishop->bishop_avail[14+i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; bish_x-i>-1; i++){ //go diag up left
		if (((bish_y-i)>-1) && ((board[bish_y-i][bish_x-i] == 0) || (board[bish_y-i][bish_x-i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->bishop_avail[21+i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->bishop_avail[21+i-1] = 0;
			for (j = 1; 21+i-1+j < 28; j++)
				bishop->bishop_avail[21+i-1+j] = 0;
			break;
		}
	}
}
void calc_rook_moves(piece *rook){
	
	char rook_x = rook->xpos;
	char rook_y = rook->ypos;
    
    int i, j;
	
	for (i = 1; rook_y-i>-1; i++){ //go up
		if ((board[rook_y-i][rook_x] == 0) || (board[rook_y-i][rook_x]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->rook_avail[i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->rook_avail[i-1] = 0;
			for (j = 1; i-1+j < 7; j++)
				rook->rook_avail[i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; rook_x+i<8; i++){ //go right
		if ((board[rook_y][rook_x+i] == 0) || (board[rook_y][rook_x+i]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->rook_avail[7+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->rook_avail[7+i-1] = 0;
			for (j = 1; 7+i-1+j < 14; j++)
				rook->rook_avail[7+i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; rook_y+i<8; i++){ //go down
		if ((board[rook_y+i][rook_x] == 0) || (board[rook_y+i][rook_x]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->rook_avail[14+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->rook_avail[14+i-1] = 0;
			for (j = 1; 14+i-1+j < 21; j++)
				rook->rook_avail[14+i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; rook_x-i>-1; i++){ //go left
		if ((board[rook_y][rook_x-i] == 0) || (board[rook_y][rook_x-i]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->rook_avail[21+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->rook_avail[21+i-1] = 0;
			for (j = 1; 21+i-1+j < 28; j++)
				rook->rook_avail[21+i-1+j] = 0;
			break;
		}
	}
}
void calc_queen_moves(piece *queen){
	
	char queen_x = queen->xpos;
	char queen_y = queen->ypos;
    
    int i, j;
	
	for (i = 1; queen_y-i>-1; i++){ //go up
		if ((board[queen_y-i][queen_x] == 0) || (board[queen_y-i][queen_x]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->queen_avail[i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->queen_avail[i-1] = 0;
			for (j = 1; i-1+j < 7; j++)
				queen->queen_avail[i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; queen_x+i<8; i++){ //go diag up right
		if ((queen_y-i)>-1){ //make sure y is in bounds
			if ((board[queen_y-i][queen_x+i] == 0) || (board[queen_y-i][queen_x+i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->queen_avail[7+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->queen_avail[7+i-1] = 0;
				for (j = 1; 7+i-1+j < 14; j++)
					queen->queen_avail[7+i-1+j] = 0;
				break;
			}
		}
	}
	
	for (i = 1; queen_x+i<8; i++){ //go right
		if ((board[queen_y][queen_x+i] == 0) || (board[queen_y][queen_x+i]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->queen_avail[14+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->queen_avail[14+i-1] = 0;
			for (j = 1; 14+i-1+j < 21; j++)
				queen->queen_avail[14+i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; queen_x+i<8; i++){ //go diag down right
		if ((queen_y+i)<8){ //make sure y is in bounds
			if ((board[queen_y+i][queen_x+i] == 0) || (board[queen_y+i][queen_x+i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->queen_avail[21+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->queen_avail[21+i-1] = 0;
				for (j = 1; 21+i-1+j < 28; j++)
					queen->queen_avail[21+i-1+j] = 0;
				break;
			}
		}
	}
	
	for (i = 1; queen_y+i<8; i++){ //go down
		if ((board[queen_y+i][queen_x] == 0) || (board[queen_y+i][queen_x]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->queen_avail[28+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->queen_avail[28+i-1] = 0;
			for (j = 1; 28+i-1+j < 35; j++)
				queen->queen_avail[28+i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; queen_x-i>-1; i++){ //go diag down left
		if ((queen_y+i)<8){ //make sure y is in bounds
			if ((board[queen_y+i][queen_x-i] == 0) || (board[queen_y+i][queen_x-i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->queen_avail[35+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->queen_avail[35+i-1] = 0;
				for (j = 1; 35+i-1+j < 42; j++)
					queen->queen_avail[35+i-1+j] = 0;
				break;
			}
		}
	}
	
	for (i = 1; queen_x-i>-1; i++){ //go left
		if ((board[queen_y][queen_x-i] == 0) || (board[queen_y][queen_x-i]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->queen_avail[42+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->queen_avail[42+i-1] = 0;
			for (j = 1; 42+i-1+j < 49; j++)
				queen->queen_avail[42+i-1+j] = 0;
			break;
		}
	}
	
	for (i = 1; queen_x-i>-1; i++){ //go diag up left
		if ((queen_y-i)>-1){ //make sure y is in bounds
			if ((board[queen_y-i][queen_x-i] == 0) || (board[queen_y-i][queen_x-i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->queen_avail[49+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->queen_avail[49+i-1] = 0;
				for (j = 1; 49+i-1+j < 56; j++)
					queen->queen_avail[49+i-1+j] = 0;
				break;
			}
		}
	}
	
}
void calc_king_moves(piece *king){
	
	char king_x = king->xpos;
	char king_y = king->ypos;
    
	//up
	if (((king_y-1) > -1) && ((board[king_y-1][king_x] == 0) || (board[king_y-1][king_x]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[0] = 1;
	else //this move is invalid
		king->king_avail[0] = 0;
	
	//up right
	if (((king_y-1)>-1) && ((king_x+1)<8) && ((board[king_y-1][king_x+1] == 0) || (board[king_y-1][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[1] = 1;
	else //this move is invalid
		king->king_avail[1] = 0;
	
	//right
	if (((king_x+1)<8) && ((board[king_y][king_x+1] == 0) || (board[king_y][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[2] = 1;
	else //this move is invalid
		king->king_avail[2] = 0;
	
	//down right
	if (((king_y+1)<8) && ((king_x+1)<8) && ((board[king_y+1][king_x+1] == 0) || (board[king_y+1][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[3] = 1;
	else //this move is invalid
		king->king_avail[3] = 0;
	
	//down
	if ((king_y+1<8) && ((board[king_y+1][king_x] == 0) || (board[king_y+1][king_x]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[4] = 1;
	else //this move is invalid
		king->king_avail[4] = 0;
	
	//down left
	if (((king_y+1)<8) && ((king_x-1)>-1) && ((board[king_y+1][king_x-1] == 0) || (board[king_y+1][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[5] = 1;
	else //this move is invalid
		king->king_avail[5] = 0;
	
	//left
	if (((king_x-1)>-1) && ((board[king_y][king_x-1] == 0) || (board[king_y][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[6] = 1;
	else //this move is invalid
		king->king_avail[6] = 0;
	
	//up left
	if (((king_y-1)>-1) && ((king_x-1)>-1) && ((board[king_y-1][king_x-1] == 0) || (board[king_y-1][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->king_avail[7] = 1;
	else //this move is invalid
		king->king_avail[7] = 0;
	
}
void calc_knight_moves(piece *knight){
	
	char knight_x = knight->xpos;
	char knight_y = knight->ypos;
	
	//big up right
	if (((knight_y-2) > -1) && ((knight_x+1)<8) && ((board[knight_y-2][knight_x+1] == 0) || (board[knight_y-2][knight_x+1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[0] = 1;
	else //this move is invalid
		knight->knight_avail[0] = 0;
		
	//small up right
	if (((knight_y-1) > -1) && ((knight_x+2)<8) && ((board[knight_y-1][knight_x+2] == 0) || (board[knight_y-1][knight_x+2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[1] = 1;
	else //this move is invalid
		knight->knight_avail[1] = 0;
	
	//small down right
	if (((knight_y+1) < 8) && ((knight_x+2)<8) && ((board[knight_y+1][knight_x+2] == 0) || (board[knight_y+1][knight_x+2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[2] = 1;
	else //this move is invalid
		knight->knight_avail[2] = 0;
		
	//big down right
	if (((knight_y+2) < 8) && ((knight_x+1)<8) && ((board[knight_y+2][knight_x+1] == 0) || (board[knight_y+2][knight_x+1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[3] = 1;
	else //this move is invalid
		knight->knight_avail[3] = 0;
	
	//big down left
	if (((knight_y+2) < 8) && ((knight_x-1)>-1) && ((board[knight_y+2][knight_x-1] == 0) || (board[knight_y+2][knight_x-1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[4] = 1;
	else //this move is invalid
		knight->knight_avail[4] = 0;
		
	//small down left
	if (((knight_y+1) < 8) && ((knight_x-2)>-1) && ((board[knight_y+1][knight_x-2] == 0) || (board[knight_y+1][knight_x-2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[5] = 1;
	else //this move is invalid
		knight->knight_avail[5] = 0;
		
	//small up left
	if (((knight_y-1) > -1) && ((knight_x-2)>-1) && ((board[knight_y-1][knight_x-2] == 0) || (board[knight_y-1][knight_x-2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[6] = 1;
	else //this move is invalid
		knight->knight_avail[6] = 0;
		
	//big up left
	if (((knight_y-2) > -1) && ((knight_x-1)>-1) && ((board[knight_y-2][knight_x-1] == 0) || (board[knight_y-2][knight_x-1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->knight_avail[7] = 1;
	else //this move is invalid
		knight->knight_avail[7] = 0;
}
void calc_piece_moves(){
	char piece_name = board[init_y][init_x];
	
	if (piece_name == PAWN)
		calc_pawn_moves(piece);
	else if (piece_name == BISHOP)
		calc_bish_moves(piece);
	else if (piece_name == KNIGHT)
		calc_knight_moves(piece);
	else if (piece_name == ROOK)
		calc_rook_moves(piece);
	else if (piece_name == QUEEN)
		calc_queen_moves(piece);
	else if (piece_name == KING)
		calc_king_moves(piece);	
    tft_fillRect(0, 30, 240, 20, ILI9340_BLACK);// x,y,w,h,color
    tft_setCursor(0, 30);
    tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
    sprintf(buffer,"%s", piece_name);
}
void capture_piece(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	if (board[new_pos_y][new_pos_x] != 0){ //store captured piece in captured array
		captured[captured_index] = board[new_pos_y][new_pos_x];
		captured_index++;
	}
	
	//now move piece to new position and set prev to 0 since it's not there anymore
	board[new_pos_y][new_pos_x] = 0;
	board[new_pos_y][new_pos_x] = board[pos_y][pos_x];
	board[pos_y][pos_x] = 0;
}
void move_pawn(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
    char move_num = 0;
	piece *piece = board[pos_y][pos_x];
    
    if ((abs(pos_x - new_pos_x) == 0) && (abs(pos_y - new_pos_y) == 2)) //2 squares up
        move_num = 1;
    else if ((abs(pos_x - new_pos_x) == 0) && (abs(pos_y - new_pos_y) == 1))//1 square up
        move_num = 2;
    else if (((pos_x - new_pos_x) == -1) && (abs(pos_y - new_pos_y) == 1)) //diag right
        move_num = 3;
    else if (((pos_x - new_pos_x) == 1) && (abs(pos_y - new_pos_y) == 1)) //diag left
        move_num = 4;
	
	if ((move_num == 1) && (piece->pawn_avail[move_num-1])){ //we want to move 2 squares to front
		piece->ypos = piece->ypos + (piece->side*2);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 2) && (piece->pawn_avail[move_num-1])){ //we want to move 1 square up
		piece->ypos = piece->ypos + piece->side;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 3) && (piece->pawn_avail[move_num-1])){//we want to go diag right
		piece->ypos = piece->ypos + piece->side;
		piece->xpos++;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 4) && (piece->pawn_avail[move_num-1])){//we want to go diag right
		piece->ypos = piece->ypos + piece->side;
		piece->xpos--;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}		
}
void move_knight(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
    char move_num = 0;
	piece *piece = board[pos_y][pos_x];
	
    if (((pos_x - new_pos_x) == -1) && ((pos_y - new_pos_y) == 2)) //big up right
        move_num = 1;
    else if (((pos_x - new_pos_x) == -2) && ((pos_y - new_pos_y) == 1)) //small up right
        move_num = 2;
    else if (((pos_x - new_pos_x) == -2) && ((pos_y - new_pos_y) == -1)) //small down right
        move_num = 3;
    else if (((pos_x - new_pos_x) == -1) && ((pos_y - new_pos_y) == -2)) //big down right
        move_num = 4;
    else if (((pos_x - new_pos_x) == 1) && ((pos_y - new_pos_y) == -2)) //big down left
        move_num = 5;
    else if (((pos_x - new_pos_x) == 2) && ((pos_y - new_pos_y) == -1)) //small down left
        move_num = 6;
    else if (((pos_x - new_pos_x) == 2) && ((pos_y - new_pos_y) == 1)) //small up left
        move_num = 7;
    else if (((pos_x - new_pos_x) == 1) && ((pos_y - new_pos_y) == 1)) //big up left
        move_num = 8;
    
	if ((move_num == 1) && (piece->knight_avail[move_num-1])){ //we want to move big up right
		piece->ypos = piece->ypos-2;
		piece->xpos = piece->xpos+1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 2) && (piece->knight_avail[move_num-1])){ //we want to move small up right
		piece->ypos = piece->ypos-1;
		piece->xpos = piece->xpos+2;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 3) && (piece->knight_avail[move_num-1])){ //we want to move small down right
		piece->ypos = piece->ypos+1;
		piece->xpos = piece->xpos+2;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 4) && (piece->knight_avail[move_num-1])){ //we want to move big down right
		piece->ypos = piece->ypos+2;
		piece->xpos = piece->xpos+1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 5) && (piece->knight_avail[move_num-1])){ //we want to move big down left
		piece->ypos = piece->ypos+2;
		piece->xpos = piece->xpos-1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 6) && (piece->knight_avail[move_num-1])){ //we want to move small down left
		piece->ypos = piece->ypos+1;
		piece->xpos = piece->xpos-2;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 7) && (piece->knight_avail[move_num-1])){ //we want to move small up left
		piece->ypos = piece->ypos-1;
		piece->xpos = piece->xpos-2;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 8) && (piece->knight_avail[move_num-1])){ //we want to move big up left
		piece->ypos = piece->ypos-2;
		piece->xpos = piece->xpos-1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}	
}
void move_king(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
    char move_num = 0;
	piece *piece = board[pos_y][pos_x];
    
    if (((pos_x - new_pos_x) == 0) && ((pos_y - new_pos_y) == 1)) //up
        move_num = 1;
    else if (((pos_x - new_pos_x) == -1) && ((pos_y - new_pos_y) == 1)) //up right
        move_num = 2;
    else if (((pos_x - new_pos_x) == -1) && ((pos_y - new_pos_y) == 0)) //right
        move_num = 3;
    else if (((pos_x - new_pos_x) == -1) && ((pos_y - new_pos_y) == -1)) //down right
        move_num = 4;
    else if (((pos_x - new_pos_x) == 0) && ((pos_y - new_pos_y) == -1)) //down
        move_num = 5;
    else if (((pos_x - new_pos_x) == 1) && ((pos_y - new_pos_y) == -1)) //down left
        move_num = 6;
    else if (((pos_x - new_pos_x) == 1) && ((pos_y - new_pos_y) == 0)) //left
        move_num = 7;
    else if (((pos_x - new_pos_x) == 1) && ((pos_y - new_pos_y) == 1)) //up left
        move_num = 8;
	
	if ((move_num == 1) && (piece->king_avail[move_num-1])){ //we want to move up front
		piece->ypos = piece->ypos-1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 2) && (piece->king_avail[move_num-1])){ //we want to move up right
		piece->ypos = piece->ypos-1;
		piece->xpos = piece->xpos+1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 3) && (piece->king_avail[move_num-1])){ //we want to move right
		piece->xpos = piece->xpos+1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 4) && (piece->king_avail[move_num-1])){ //we want to move down right
		piece->ypos = piece->ypos+1;
		piece->xpos = piece->xpos+1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 5) && (piece->king_avail[move_num-1])){ //we want to move down
		piece->ypos = piece->ypos+1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 6) && (piece->king_avail[move_num-1])){ //we want to move down left
		piece->ypos = piece->ypos+1;
		piece->xpos = piece->xpos-1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 7) && (piece->king_avail[move_num-1])){ //we want to move left
		piece->xpos = piece->xpos-1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num == 8) && (piece->king_avail[move_num-1])){ //we want to move up left
		piece->ypos = piece->ypos-1;
		piece->xpos = piece->xpos-1;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}	
}
void move_queen(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
    char move_num = 57;
	piece *piece = board[pos_y][pos_x];
    
    char xdiff = pos_x - new_pos_x;
    char ydiff = pos_y - new_pos_y;
    
    if ((xdiff == 0) && (ydiff > 0)) //up
        move_num = ydiff;
    else if ((xdiff < 0) && (ydiff > 0) && (xdiff == -ydiff)) //up right
        move_num = 8 + ydiff;
    else if ((xdiff < 0) && (ydiff == 0)) //right
        move_num = 15 - xdiff;
    else if ((xdiff < 0) && (ydiff < 0) && (xdiff == ydiff)) //down right
        move_num = 22 - ydiff;
    else if ((xdiff == 0) && (ydiff < 0)) //down
        move_num = 29 - ydiff;
    else if ((xdiff > 0) && (ydiff < 0) && (xdiff == -ydiff)) //down left
        move_num = 36 + xdiff;
    else if ((xdiff > 0) && (ydiff == 0)) //left
        move_num = 43 + xdiff;
    else if ((xdiff > 0) && (ydiff > 0) && (xdiff == ydiff)) //up left
        move_num = 50 + ydiff;
	
	if ((move_num < 8) && (piece->queen_avail[move_num-1])){ //we want to move up line
		piece->ypos = piece->ypos-move_num;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 15) && (piece->queen_avail[move_num-1])){ //we want to move diag up right
		piece->ypos = piece->ypos-(move_num-7);
		piece->xpos = piece->xpos+(move_num-7);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 22) && (piece->queen_avail[move_num-1])){ //we want to move right
		piece->xpos = piece->xpos+(move_num-14);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 29) && (piece->queen_avail[move_num-1])){ //we want to move down right
		piece->ypos = piece->ypos+(move_num-21);
		piece->xpos = piece->xpos+(move_num-21);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 36) && (piece->queen_avail[move_num-1])){ //we want to move down
		piece->ypos = piece->ypos+(move_num-28);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 43) && (piece->queen_avail[move_num-1])){ //we want to move down left
		piece->ypos = piece->ypos+(move_num-21);
		piece->xpos = piece->xpos-(move_num-21);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 50) && (piece->queen_avail[move_num-1])){ //we want to move left
		piece->xpos = piece->xpos-(move_num-42);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 57) && (piece->queen_avail[move_num-1])){ //we want to move up left
		piece->ypos = piece->ypos-(move_num-21);
		piece->xpos = piece->xpos-(move_num-21);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}	
}
void move_rook(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
	piece *piece = board[pos_y][pos_x];
	
    char move_num = 29;
    char xdiff = pos_x - new_pos_x;
    char ydiff = pos_y - new_pos_y;
    
    if ((xdiff == 0) && (ydiff > 0)) //up
        move_num = ydiff;
    else if ((xdiff < 0) && (ydiff == 0)) //right
        move_num = 8 - xdiff;
    else if ((xdiff == 0) && (ydiff < 0)) //down
        move_num = 15 - ydiff;
    else if ((xdiff > 0) && (ydiff == 0)) //left
        move_num = 22 + xdiff;
    
	if ((move_num < 8) && (piece->rook_avail[move_num-1])){ //we want to move up line
		piece->ypos = piece->ypos+move_num;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 15) && (piece->rook_avail[move_num-1])){ //we want to move right
		piece->xpos = piece->xpos+(move_num-7);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 22) && (piece->rook_avail[move_num-1])){ //we want to move down
		piece->ypos = piece->ypos-(move_num-14);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 29) && (piece->rook_avail[move_num-1])){ //we want to move left
		piece->xpos = piece->xpos-(move_num-21);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
}
void move_bishop(char pos_x, char pos_y, char new_pos_x, char new_pos_y){
	
	piece *piece = board[pos_y][pos_x];
    
    char move_num = 29;    
    char xdiff = pos_x - new_pos_x;
    char ydiff = pos_y - new_pos_y;
    
    if ((xdiff < 0) && (ydiff > 0) && (xdiff == -ydiff)) //up right
        move_num = ydiff;
    else if ((xdiff < 0) && (ydiff < 0) && (xdiff == ydiff)) //down right
        move_num = 8 - ydiff;
    else if ((xdiff > 0) && (ydiff < 0) && (xdiff == -ydiff)) //down left
        move_num = 15 + xdiff;
    else if ((xdiff > 0) && (ydiff > 0) && (xdiff == ydiff)) //up left
        move_num = 22 + ydiff;
	
	if ((move_num < 8) && (piece->bishop_avail[move_num-1])){ //we want to move diag up right
		piece->ypos = piece->ypos-move_num;
		piece->xpos = piece->xpos+move_num;
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 15) && (piece->bishop_avail[move_num-1])){ //we want to move down right
		piece->ypos = piece->ypos+(move_num-7);
		piece->xpos = piece->xpos+(move_num-7);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 22) && (piece->bishop_avail[move_num-1])){ //we want to move down left
		piece->ypos = piece->ypos+(move_num-14);
		piece->xpos = piece->xpos-(move_num-14);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}
	else if ((move_num < 29) && (piece->bishop_avail[move_num-1])){ //we want to move up left
		piece->ypos = piece->ypos-(move_num-21);
		piece->xpos = piece->xpos-(move_num-21);
		capture_piece(pos_x, pos_y, piece->xpos, piece->ypos);
	}	
}
void move_piece(){
	char piece_name = board[init_y][init_x]->name;
	
	if (piece_name == PAWN)
		move_pawn(init_x, init_y, end_x, end_y);
    else if (piece_name == BISHOP)
		move_bishop(init_x, init_y, end_x, end_y);
	else if (piece_name == KNIGHT)
		move_knight(init_x, init_y, end_x, end_y);
	else if (piece_name == ROOK)
		move_rook(init_x, init_y, end_x, end_y);
	else if (piece_name == QUEEN)
		move_queen(init_x, init_y, end_x, end_y);
	else if (piece_name == KING)
		move_king(init_x, init_y, end_x, end_y);
	
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
            //printf("%i\t", init_x);
            printf("%i%1i\t %i%1i\n\r", init_x, init_y, end_x, end_y);      
        else if(cmd[0] >= 97 && cmd[0] <= 104 && cmd[1] >= 48 && cmd[1] <= 56)   {
            if(state == IDLE)   {
                init_x = cmd[0] - 97;
                init_y = cmd[1] - 49;
                state++;
            }
            else if (state == CALC) {
                end_x = cmd[0] - 97;
                end_y = cmd[1] - 49;
                state = 0;
            }
        }
    } // END WHILE(1)
    PT_END(pt);
}

// === TFT Print Thread =================================================
// print the output of the chessboard
static PT_THREAD (protothread_timer(struct pt *pt)){
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
            tft_fillRect(0, 10, 240, 20, ILI9340_BLACK);// x,y,w,h,color
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
                }
            }
            // NEVER exit while
        } // END WHILE(1)
  PT_END(pt);
} // timer thread
// === Calculate Move and Make Move Thread =============================================
static PT_THREAD (protothread_move(struct pt *pt)){
    PT_BEGIN(pt);
        while(1) {       
          // if button or command is entered for both start and end, make the move
            if(state == IDLE);
                //calc_piece_moves();
            else if(state == CALC);
                //move_piece();
        } // END WHILE(1)
	PT_END(pt);
} // animation thread

// === Main  ======================================================
void main(void) {
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_timer);
  PT_INIT(&pt_key);
  //PT_INIT(&pt_board);
  PT_INIT(&pt_move);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  
  initialize_board();

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_keyboard(&pt_key));
      PT_SCHEDULE(protothread_timer(&pt_timer));
      //PT_SCHEDULE(protothread_board(&pt_board));
      PT_SCHEDULE(protothread_move(&pt_move));
  }
 } // main

// === end  ======================================================

