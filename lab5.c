#include <stdlib.h>
#include "config.h"
#define use_uart_serial
#include "pt_cornell_1_2.h"
#include "tft_master.h"
#include "tft_gfx.h"

#define PAWN   0
#define BISHOP 1
#define KNIGHT 2
#define ROOK   3
#define QUEEN  4
#define KING   5

char buffer[60];
static char cmd[1];
static struct pt pt_timer, pt_key, pt_color, pt_anim ;
int sys_time_seconds;

typedef struct pawn{
	int avail_moves[4]; //pawn has max 3 moves avail. forward or diag to capture.
}pawn;

typedef struct bishop{
	int avail_moves[28];//bishop can move 7 squares diagonally
}bishop;

typedef struct knight{
	int avail_moves[8]; //knight can move L in all directions
};

typedef struct rook{
	int avail_moves[28];//king can move seven square in perpendicular
}rook;

typedef struct queen{
	int avail_moves[56];//queen can move seven squares in all directions
}queen;

typedef struct king{
	int avail_moves[8]; //king can move one square in all directions
}king;

typedef struct piece{
	
	int name;   //what piece it is
	int start;  //0 means moved this game
	int alive;  //alive = 1, it's alive. 0 = dead
	int side;   //side = 1, it's white. -1 = black
    int xpos;   //ranges between a-h (a = 0, h = 7)
    int ypos;   //ranges between 1 and 8 (1 = 0, 8 = 7)
	
	typedef union{
		pawn   pawn_avail;
		bishop bishop_avail;
		knight knight_avail;
		rook   rook_avail;
		queen  queen_avail;
		king   king_avail;
	}avail_moves;
}

// our 8x8 board of pieces
piece *board[8][8] = {0};
piece *captured[29] = {0}; //captured elements
int captured_index = 0;

piece *selected;

piece pawn;
piece rook;
piece knight;
piece bishop;
piece queen;
piece king;
int side_loop = -1;

volatile int button_x;
volatile int button_y;
volatile int button_move;

void initialize_board(){
		
	//initialize white pawns
	for (int j = 1; 1 < 8; j+=5){
		side_loop = side_loop * -1;
		for (int i = 0; i < 8; i++){
			pawn = (piece*)malloc(sizeof(piece));
			board[j][i] = pawn;
			pawn->name = PAWN;
			pawn->start = 1;
			pawn->alive = 1;
			pawn->side = side_loop;
			pawn->xpos = i;
			pawn->ypos = j;
			pawn->avail_moves = malloc(sizeof(avail_moves));
			pawn->avail_moves.pawn_avail = malloc(sizeof(pawn));
		}	
	}
	
	side_loop = -1;
	
	//initialize rooks
	for (int j = 0; j < 8; j+=6){
		side_loop = side_loop * -1;
		for (int i = 0; i < 8; i+=7){
			rook = (piece*)malloc(sizeof(piece));
			board[j][i] = rook;
			rook->name = ROOK;
			rook->start = 1;
			rook->alive = 1;
			rook->side = side_loop;
			rook->xpos = i;
			rook->ypos = j;
			rook->avail_moves = malloc(sizeof(avail_moves));
			rook->avail_moves.rook_avail = malloc(sizeof(rook));
		}
	}
	
	side_loop = -1;
	
	//initialize knights
	for (int j = 0; j < 8; j+=6){
		side_loop = side_loop * -1;
		for (int i = 1; i < 8; i+=5){
			knight = (piece*)malloc(sizeof(piece));
			board[j][i] = knight;
			knight->name = KNIGHT;
			knight->start = 1;
			knight->alive = 1;
			knight->side = side_loop;
			knight->xpos = i;
			knight->ypos = j;
			knight->avail_moves = malloc(sizeof(avail_moves));
			knight->avail_moves.knight_avail = malloc(sizeof(knight));
		}
	}
	
	side_loop = -1;
	
	//initialize bishop
	for (int j = 0; j < 8; j+=6){
		side_loop = side_loop * -1;
		for (int i = 2; i < 8; i+=3){
			bishop = (piece*)malloc(sizeof(piece));
			board[j][i] = bishop;
			bishop->name = BISHOP;
			bishop->start = 1;
			bishop->alive = 1;
			bishop->side = side_loop;
			bishop->xpos = i;
			bishop->ypos = j;
			bishop->avail_moves = malloc(sizeof(avail_moves));
			bishop->avail_moves.bishop_avail = malloc(sizeof(bishop));
		}
	}
	
	side_loop = -1;
	
	//initialize queen
	for (int j = 0; j < 8; j+=6){
		side_loop = side_loop * -1;
		queen = (piece*)malloc(sizeof(piece));
		board[j][3] = queen;
		queen->name = QUEEN;
		queen->start = 1;
		queen->alive = 1;
		queen->side = side_loop;
		queen->ypos = 3;
		queen->xpos = j;
		queen->avail_moves = malloc(sizeof(avail_moves));
		queen->avail_moves.queen_avail = malloc(sizeof(queen));
	}
	
	side_loop = -1;
	
	//initialize king
	for (int j = 0; j < 8; j+=6){
		side_loop = side_loop * -1;
		king = (piece*)malloc(sizeof(piece));
		board[j][4] = king;
		king->name = KING;
		king->start = 1;
		king->alive = 1;
		king->side = side_loop;
		king->xpos = 4;
		king->ypos = j;
		king->avail_moves = malloc(sizeof(avail_moves));
		king->avail_moves.king_avail = malloc(sizeof(king));
	}
	
}

void calc_pawn_moves(piece *pawn){
	
	int pawn_x = pawn->xpos;
	int pawn_y = pawn->ypos;
	
	int pawn_y_sideff = pawn->ypos + (pawn->side * 2);
	int pawn_y_sidef = pawn->ypos + (pawn->side * 1);
	int pawn_x_sider = pawn->xpos + 1;
	int pawn_x_sidel = pawn->xpos - 1;
		
	//two squares up
	if (((pawn->start) && (board[pawn_y_sideff][pawn_x] == 0)) //no piece should be there
		pawn->avail_moves.pawn_avail[0] = 1;
	else
		pawn->avail_moves.pawn_avail[0] = 0;
	
	//diag right
	if (((pawn_x_sidedr)<8) && (board[pawn_y_sidef][pawn_x_sider] != 0) && (board[pawn_y_sidef][pawn_x_sider]->side != pawn->side)) //see if piece is there
		pawn->avail_moves.pawn_avail[2] = 1;
	else
		pawn->avail_moves.pawn_avail[2] = 0;

	//diag left
	if (((pawn_x_sidedl)>-1) && (board[pawn_y_sidef][pawn_x_sidel] != 0) && (board[pawn_y_sidef][pawn_x_sidel]->side != pawn->side)) //see if enemy piece is there
		pawn->avail_moves.pawn_avail[3] = 1;
	else
		pawn->avail_moves.pawn_avail[3] = 0;
	
	//one square up
	if (board[pawn_y_sidef][pawn_x_sidel] == 0) //no piece should be in front of pawn
		pawn->avail_moves.pawn_avail[1] = 1;
	else
		pawn->avail_moves.pawn_avail[1] = 0;
	
}

void calc_bish_moves(piece *bishop){
	
	int bish_x = bishop->xpos;
	int bish_y = bishop->ypos;
	
	for (int i = 1; bish_x+i<8; i++){ //go diag up right
		if (((bish_y+i)<8) && ((board[bish_y+i][bish_x+i] == 0) || (board[bish_y+i][bish_x+i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->avail_moves.bishop_avail[i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->avail_moves.bishop_avail[i-1] = 0;
			for (int j = 1; i-1+j < 7; j++)
				bishop->avail_moves.bishop_avail[i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; bish_x+i<8; i++){ //go diag down right
		if (((bish_y-i)>-1) && ((board[bish_y-i][bish_x+i] == 0) || (board[bish_y-i][bish_x+i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->avail_moves.bishop_avail[7+i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->avail_moves.bishop_avail[7+i-1] = 0;
			for (int j = 1; 7+i-1+j < 14; j++)
				bishop->avail_moves.bishop_avail[7+i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; bish_x-i>-1; i++){ //go diag down left
		if (((bish_y-i)>-1) && ((board[bish_y-i][bish_x-i] == 0) || (board[bish_y-i][bish_x-i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->avail_moves.bishop_avail[14+i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->avail_moves.bishop_avail[14+i-1] = 0;
			for (int j = 1; 14+i-1+j < 21; j++)
				bishop->avail_moves.bishop_avail[14+i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; bish_x-i>-1; i++){ //go diag up left
		if (((bish_y+i)<8) && ((board[bish_y+i][bish_x-i] == 0) || (board[bish_y+i][bish_x-i]->side != bishop->side))) //make sure no piece there or it's enemy piece
			bishop->avail_moves.bishop_avail[21+i-1] = 1;
		else{ //this move is invalid. so are rest in diagonal
			bishop->avail_moves.bishop_avail[21+i-1] = 0;
			for (int j = 1; 21+i-1+j < 28; j++)
				bishop->avail_moves.bishop_avail[21+i-1+j] = 0;
			break;
		}
	}
}

void calc_rook_moves(piece *rook){
	
	int rook_x = rook->xpos;
	int rook_y = rook->ypos;
	
	for (int i = 1; rook_y+i<8; i++){ //go up
		if ((board[rook_y+i][rook_x] == 0) || (board[rook_y+i][rook_x]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->avail_moves.rook_avail[i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->avail_moves.rook_avail[i-1] = 0;
			for (int j = 1; i-1+j < 7; j++)
				rook->avail_moves.rook_avail[i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; rook_x+i<8; i++){ //go right
		if ((board[rook_y][rook_x+i] == 0) || (board[rook_y][rook_x+i]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->avail_moves.rook_avail[7+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->avail_moves.rook_avail[7+i-1] = 0;
			for (int j = 1; 7+i-1+j < 14; j++)
				rook->avail_moves.rook_avail[7+i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; rook_y-i>-1; i++){ //go down
		if ((board[rook_y-i][rook_x] == 0) || (board[rook_y-i][rook_x]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->avail_moves.rook_avail[14+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->avail_moves.rook_avail[14+i-1] = 0;
			for (int j = 1; 14+i-1+j < 21; j++)
				rook->avail_moves.rook_avail[14+i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; rook_x-i>-1; i++){ //go left
		if ((board[rook_y][rook_x-i] == 0) || (board[rook_y][rook_x-i]->side != rook->side)) //make sure no piece there or it's enemy piece
			rook->avail_moves.rook_avail[21+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			rook->avail_moves.rook_avail[21+i-1] = 0;
			for (int j = 1; 21+i-1+j < 28; j++)
				rook->avail_moves.rook_avail[21+i-1+j] = 0;
			break;
		}
	}
}

void calc_queen_moves(piece *queen){
	
	int queen_x = queen->xpos;
	int queen_y = queen->ypos;
	
	for (int i = 1; queen_y+i<8; i++){ //go up
		if ((board[queen_y+i][queen_x] == 0) || (board[queen_y+i][queen_x]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->avail_moves.queen_avail[i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->avail_moves.queen_avail[i-1] = 0;
			for (int j = 1; i-1+j < 7; j++)
				queen->avail_moves.queen_avail[i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; queen_x+i<8; i++){ //go diag up right
		if ((queen_y+i)<8){ //make sure y is in bounds
			if ((board[queen_y+i][queen_x+i] == 0) || (board[queen_y+i][queen_x+i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->avail_moves.queen_avail[7+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->avail_moves.queen_avail[7+i-1] = 0;
				for (int j = 1; 7+i-1+j < 14; j++)
					queen->avail_moves.queen_avail[7+i-1+j] = 0;
				break;
			}
		}
	}
	
	for (int i = 1; rook_x+i<8; i++){ //go right
		if ((board[queen_y][queen_x+i] == 0) || (board[queen_y][queen_x+i]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->avail_moves.queen_avail[14+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->avail_moves.queen_avail[14+i-1] = 0;
			for (int j = 1; 14+i-1+j < 21; j++)
				queen->avail_moves.queen_avail[14+i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; queen_x+i<8; i++){ //go diag down right
		if ((queen_y-i)>-1){ //make sure y is in bounds
			if ((board[queen_y-i][queen_x+i] == 0) || (board[queen_y-i][queen_x+i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->avail_moves.queen_avail[21+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->avail_moves.queen_avail[21+i-1] = 0;
				for (int j = 1; 21+i-1+j < 28; j++)
					queen->avail_moves.queen_avail[21+i-1+j] = 0;
				break;
			}
		}
	}
	
	for (int i = 1; queen_y-i>-1; i++){ //go down
		if ((board[queen_y-i][queen_x] == 0) || (board[queen_y-i][queen_x]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->avail_moves.queen_avail[28+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->avail_moves.queen_avail[28+i-1] = 0;
			for (int j = 1; 28+i-1+j < 35; j++)
				queen->avail_moves.queen_avail[28+i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; queen_x-i>-1; i++){ //go diag down left
		if ((queen_y-i)>-1){ //make sure y is in bounds
			if ((board[queen_y-i][queen_x-i] == 0) || (board[queen_y-i][queen_x-i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->avail_moves.queen_avail[35+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->avail_moves.queen_avail[35+i-1] = 0;
				for (int j = 1; 35+i-1+j < 42; j++)
					queen->avail_moves.queen_avail[35+i-1+j] = 0;
				break;
			}
		}
	}
	
	for (int i = 1; queen_x-i>-1; i++){ //go left
		if ((board[queen_y][queen_x-i] == 0) || (board[queen_y][queen_x-i]->side != queen->side)) //make sure no piece there or it's enemy piece
			queen->avail_moves.queen_avail[42+i-1] = 1;
		else{ //this move is invalid. so are rest in line
			queen->avail_moves.queen_avail[42+i-1] = 0;
			for (int j = 1; 42+i-1+j < 49; j++)
				queen->avail_moves.queen_avail[42+i-1+j] = 0;
			break;
		}
	}
	
	for (int i = 1; queen_x-i>-1; i++){ //go diag up left
		if ((queen_y+i)<8){ //make sure y is in bounds
			if ((board[queen_y+i][queen_x-i] == 0) || (board[queen_y+i][queen_x-i]->side != queen->side)) //make sure no piece there or it's enemy piece
				queen->avail_moves.queen_avail[49+i-1] = 1;
			else{ //this move is invalid. so are rest in diagonal
				queen->avail_moves.queen_avail[49+i-1] = 0;
				for (int j = 1; 49+i-1+j < 56; j++)
					queen->avail_moves.queen_avail[49+i-1+j] = 0;
				break;
			}
		}
	}
	
}

void calc_king_moves(piece *king){
	
	int king_x = king->xpos;
	int king_y = king->ypos;
	
	//up
	if (((king_y+1) < 8) && ((board[king_y+1][king_x] == 0) || (board[king_y+1][king_x]->side != king->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.king_avail[0] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[0] = 0;
	
	//up right
	if (((king_y+1)<8) && ((king_x+1)<8) && ((board[king_y+1][king_x+1] == 0) || (board[king_y+1][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.king_avail[1] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[1] = 0;
	
	//right
	if (((king_x+1)<8) && ((board[king_y][king_x+i] == 0) || (board[king_y][king_x+i]->side != king->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.queen_avail[2] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[2] = 0;
	
	//down right
	if (((king_y-1)>-1) && ((king_x+1)<8) && ((board[king_y-1][king_x+1] == 0) || (board[king_y-1][king_x+1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.king_avail[3] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[3] = 0;
	
	//down
	if ((king_y-i>-1) && ((board[king_y-1][king_x] == 0) || (board[king_y-1][king_x]->side != qking->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.king_avail[4] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[4] = 0;
	
	//down left
	if (((king_y-1)>-1) && ((king_x-1)>-1) && ((board[king_y-1][king_x-1] == 0) || (board[king_y-1][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.king_avail[5] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[5] = 0;
	
	//left
	if (((king_x-1)>-1) && ((board[king_y][king_x-i] == 0) || (board[king_y][king_x-i]->side != king->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.queen_avail[6] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[6] = 0;
	
	//up left
	if (((king_y+1)<8) && ((king_x-1)>-1) && ((board[king_y+1][king_x-1] == 0) || (board[king_y+1][king_x-1]->side != king->side))) //make sure no piece there or it's enemy piece
		king->avail_moves.king_avail[7] = 1;
	else //this move is invalid
		king->avail_moves.king_avail[7] = 0;
	
}

void calc_knight_moves(piece *knight){
	
	int knight_x = knight->xpos;
	int knight_y = knight->ypos;
	
	//big up right
	if (((knight_y+2) < 8) && ((knight_x+1)<8) && ((board[knight_y+2][knight_x+1] == 0) || (board[knight_y+2][knight_x+1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[0] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[0] = 0;
		
	//small up right
	if (((knight_y+1) < 8) && ((knight_x+2)<8) && ((board[knight_y+1][knight_x+2] == 0) || (board[knight_y+1][knight_x+2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[1] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[1] = 0;
	
	//small down right
	if (((knight_y+1) < 8) && ((knight_x-2)>-1) && ((board[knight_y+1][knight_x-2] == 0) || (board[knight_y+1][knight_x-2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[2] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[2] = 0;
		
	//big down right
	if (((knight_y-2) > -1) && ((knight_x+1)<8) && ((board[knight_y-2][knight_x+1] == 0) || (board[knight_y-2][knight_x+1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[3] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[3] = 0;
	
	//big down left
	if (((knight_y-2) > -1) && ((knight_x-1)>-1) && ((board[knight_y-2][knight_x-1] == 0) || (board[knight_y-2][knight_x-1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[4] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[4] = 0;
		
	//small down left
	if (((knight_y-1) > -1) && ((knight_x-2)>-1) && ((board[knight_y-1][knight_x-2] == 0) || (board[knight_y-1][knight_x-2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[5] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[5] = 0;
		
	//small up left
	if (((knight_y+1) < 8) && ((knight_x-2)>-1) && ((board[knight_y+1][knight_x-2] == 0) || (board[knight_y+1][knight_x-2]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[6] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[6] = 0;
		
	//big up left
	if (((knight_y+2) < 8) && ((knight_x-1)>-1) && ((board[knight_y+2][knight_x-1] == 0) || (board[knight_y+2][knight_x-1]->side != knight->side))) //make sure no piece there or it's enemy piece
		knight->avail_moves.knight_avail[7] = 1;
	else //this move is invalid
		knight->avail_moves.knight_avail[7] = 0;
}

void calc_piece_moves(int pos_x, int pos_y){
	
	piece *piece = board[pos_y][pos_x];
	int piece_name = piece->name;
	
	if (piece_name == PAWN)
		calc_pawn_moves(pos_x, pos_y, move_num);
	else if (piece_name == BISHOP)
		calc_bish_moves(pos_x, pos_y, move_num);
	else if (piece_name == KNIGHT)
		calc_knight_moves(pos_x, pos_y, move_num);
	else if (piece_name == ROOK)
		calc_rook_moves(pos_x, pos_y, move_num);
	else if (piece_name == QUEEN)
		calc_queen_moves(pos_x, pos_y, move_num);
	else if (piece_name == KING)
		calc_king_moves(pos_x, pos_y, move_num);
	
}

void capture_piece(int pos_x, int pos_y, int new_pos_x, int new_pos_y){
	if board[new_pos_y][new_pos_x] != 0{ //store captured piece in captured array
		captured[captured_index] = board[new_pos_y][new_pos_x];
		captured_index++;
	}
	
	//now move piece to new position and set prev to 0 since it's not there anymore
	board[new_pos_y][new_pos_x] = 0;
	board[new_pos_y][new_pos_x] = board[pos_y][pos_x];
	board[pos_y][pos_x] = 0;
}

void move_pawn(int pos_x, int pos_y, int move_num){
	
	piece *piece = board[pos_y][pos_x];
	
	if ((move_num == 1) && (piece->moves_avail.pawn_avail[move_num-1])){ //we want to move 2 squares to front
		piece->pos_y = piece->pos_y + (piece->side*2);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 2) && (piece->moves_avail.pawn_avail[move_num-1])){ //we want to move 1 square up
		piece->pos_y = piece->pos_y + piece->side;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 3) && (piece->moves_avail.pawn_avail[move_num-1])){//we want to go diag right
		piece->pos_y = piece->pos_y + piece->side;
		piece->pos_x++;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 4) && (piece->moves_avail.pawn_avail[move_num-1])){//we want to go diag right
		piece->pos_y = piece->pos_y + piece->side;
		piece->pos_x--;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}		
}

void move_knight(int pos_x, int pos_y, int move_num){
	
	piece *piece = board[pos_y][pos_x];
	
	if ((move_num == 1) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move big up right
		piece->pos_y = piece->pos_y+2;
		piece->pos_x = piece->pos_x+1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 2) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move small up right
		piece->pos_y = piece->pos_y+1;
		piece->pos_x = piece->pos_x+2;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 3) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move small down right
		piece->pos_y = piece->pos_y-1;
		piece->pos_x = piece->pos_x+2;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 4) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move big down right
		piece->pos_y = piece->pos_y-2;
		piece->pos_x = piece->pos_x+1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 5) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move big down left
		piece->pos_y = piece->pos_y-2;
		piece->pos_x = piece->pos_x-1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 6) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move small down left
		piece->pos_y = piece->pos_y-1;
		piece->pos_x = piece->pos_x-2;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 7) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move small up left
		piece->pos_y = piece->pos_y+1;
		piece->pos_x = piece->pos_x-2;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 8) && (piece->moves_avail.knight_avail[move_num-1])){ //we want to move big up left
		piece->pos_y = piece->pos_y+2;
		piece->pos_x = piece->pos_x-1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}	
}

void move_king(int pos_x, int pos_y, int move_num){
	
	piece *piece = board[pos_y][pos_x];
	
	if ((move_num == 1) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move up front
		piece->pos_y = piece->pos_y+1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 2) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move up right
		piece->pos_y = piece->pos_y+1;
		piece->pos_x = piece->pos_x+1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 3) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move right
		piece->pos_x = piece->pos_x+1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 4) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move down right
		piece->pos_y = piece->pos_y-1;
		piece->pos_x = piece->pos_x+1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 5) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move down
		piece->pos_y = piece->pos_y-1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 6) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move down left
		piece->pos_y = piece->pos_y-1;
		piece->pos_x = piece->pos_x-1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 7) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move left
		piece->pos_x = piece->pos_x-1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num == 8) && (piece->moves_avail.king_avail[move_num-1])){ //we want to move up left
		piece->pos_y = piece->pos_y+1;
		piece->pos_x = piece->pos_x-1;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}	
}

void move_queen(int pos_x, int pos_y, int move_num){
	
	piece *piece = board[pos_y][pos_x];
	
	if ((move_num < 8) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move up line
		piece->pos_y = piece->pos_y+move_num;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 15) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move diag up right
		piece->pos_y = piece->pos_y+(move_num-7);
		piece->pos_x = piece->pos_x+(move_num-7);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 22) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move right
		piece->pos_x = piece->pos_x+(move_num-14);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 29) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move down right
		piece->pos_y = piece->pos_y-(move_num-21);
		piece->pos_x = piece->pos_x+(move_num-21);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 36) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move down
		piece->pos_y = piece->pos_y-(move_num-28);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 43) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move down left
		piece->pos_y = piece->pos_y-(move_num-21);
		piece->pos_x = piece->pos_x-(move_num-21);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 50) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move left
		piece->pos_x = piece->pos_x-(move_num-42);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 57) && (piece->moves_avail.queen_avail[move_num-1])){ //we want to move up left
		piece->pos_y = piece->pos_y+(move_num-21);
		piece->pos_x = piece->pos_x-(move_num-21);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}	
}

void move_rook(int pos_x, int pos_y, int move_num){
	
	piece *piece = board[pos_y][pos_x];
	
	if ((move_num < 8) && (piece->moves_avail.rook_avail[move_num-1])){ //we want to move up line
		piece->pos_y = piece->pos_y+move_num;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 15) && (piece->moves_avail.rook_avail[move_num-1])){ //we want to move right
		piece->pos_x = piece->pos_x+(move_num-7);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 22) && (piece->moves_avail.rook_avail[move_num-1])){ //we want to move down
		piece->pos_y = piece->pos_y-(move_num-14);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 29) && (piece->moves_avail.rook_avail[move_num-1])){ //we want to move left
		piece->pos_x = piece->pos_x-(move_num-21);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
}

void move_bishop(int pos_x, int pos_y, int move_num){
	
	piece *piece = board[pos_y][pos_x];
	
	if ((move_num < 8) && (piece->moves_avail.bishop_avail[move_num-1])){ //we want to move diag up right
		piece->pos_y = piece->pos_y+move_num;
		piece->pos_x = piece->pos_x+move_num;
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 15) && (piece->moves_avail.bishop_avail[move_num-1])){ //we want to move down right
		piece->pos_y = piece->pos_y-(move_num-7);
		piece->pos_x = piece->pos_x+(move_num-7);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 22) && (piece->moves_avail.bishop_avail[move_num-1])){ //we want to move down left
		piece->pos_y = piece->pos_y-(move_num-14);
		piece->pos_x = piece->pos_x-(move_num-14);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}
	else if ((move_num < 29) && (piece->moves_avail.bishop_avail[move_num-1])){ //we want to move up left
		piece->pos_y = piece->pos_y+(move_num-21);
		piece->pos_x = piece->pos_x-(move_num-21);
		capture_piece(pos_x, pos_y, piece->pos_x, piece->pos_y);
	}	
}

void move_piece(int pos_x, int pos_y, int move_num){
	
	piece *piece = board[pos_y][pos_x];
	int piece_name = piece->name;
	
	if (piece_name == PAWN)
		move_pawn(pos_x, pos_y, move_num);
	else if (piece_name == BISHOP)
		move_bishop(pos_x, pos_y, move_num);
	else if (piece_name == KNIGHT)
		move_knight(pos_x, pos_y, move_num);
	else if (piece_name == ROOK)
		move_rook(pos_x, pos_y, move_num);
	else if (piece_name == QUEEN)
		move_queen(pos_x, pos_y, move_num);
	else if (piece_name == KING)
		move_king(pos_x, pos_y, move_num);
	
}




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
        // returns when the thead dies
        // in this case, when <enter> is pushed
        // now parse the string
         sscanf(PT_term_buffer, "%s %lf", cmd, &value);
         switch(cmd[0]){
            case 's': // command form: w
                //desired_motor_speed_ticks = value;
                break;
            case 'p': // command form: w
                //pid_prop_gain = value;
                break;
            case 'i': // command form: w
                //pid_int_gain = value;
                break;
            case 'd': // command form: w
                //pid_diff_gain = value;
                break;
             case 'z':
                //printf("%.1f\t%.1f\t%.1f\t%.1f\n\r", desired_motor_speed_ticks, pid_prop_gain, pid_int_gain, pid_diff_gain);
                break;
         }
            // never exit while
      } // END WHILE(1)
    PT_END(pt);
}
// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt)){
    PT_BEGIN(pt);
     tft_setCursor(0, 0);
     tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
     tft_writeString("Time in seconds since boot\n");
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        
        // draw sys_time
        tft_fillRoundRect(0,10, 100, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 10);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer);
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Color Thread =================================================
// draw 3 color patches for R,G,B from a random number
static int color ;
static int i;
static PT_THREAD (protothread_color(struct pt *pt)){
    PT_BEGIN(pt);
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(2000) ;

        // choose a random color
        color = rand() & 0xffff ;
       
        // draw color string
        tft_fillRoundRect(0,50, 150, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 50);
        tft_setTextColor(ILI9340_WHITE); tft_setTextSize(1);
        sprintf(buffer," %04x  %04x  %04x  %04x", color & 0x1f, color & 0x7e0, color & 0xf800, color);
        tft_writeString(buffer);

        // draw the actual color patches
        tft_fillRoundRect(5,70, 30, 30, 1, color & 0x1f);// x,y,w,h,radius,blues
        tft_fillRoundRect(40,70, 30, 30, 1, color & 0x7e0);// x,y,w,h,radius,greens
        tft_fillRoundRect(75,70, 30, 30, 1, color & 0xf800);// x,y,w,h,radius,reds
        // now draw the RGB mixed color
        tft_fillRoundRect(110,70, 30, 30, 1, color);// x,y,w,h,radius,mix color
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // color thread

// === Calculate Move and Make Move Thread =============================================
// update a 1 second tick counter
static PT_THREAD (protothread_anim(struct pt *pt)){
    PT_BEGIN(pt);
      while(1) {

		//on button press, global pos_x and global pos_y and global move should be set
		selected = board[button_y][button_x];
		calc_piece_moves(selected);
		move_piece(button_x, button_y, button_move);
		
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
  PT_INIT(&pt_color);
  PT_INIT(&pt_anim);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);
  
  initialize_board();

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_keyboard(&pt_key));
      PT_SCHEDULE(protothread_timer(&pt_timer));
      //PT_SCHEDULE(protothread_color(&pt_color));
      //PT_SCHEDULE(protothread_anim(&pt_anim));
  }
 } // main

// === end  ======================================================

