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
#define SHIFT  -20

static double value;
static char cmd[2], signal = 1;
static struct pt pt_end, pt_out, pt_but, pt_key, pt_time, pt_move, pt_input, pt_output, pt_DMA_output ;
static struct pt pt_blink;
int white_time_seconds = 300, black_time_seconds = 300, state = -1;
char buffer[60], init_x, init_y, end_x, end_y, current_player = 1; //positions that will be selected by user via buttons
char white_king_taken = 0, black_king_taken = 0;
char is_row_pressed = 0, is_column_pressed = 0;
static char row_input = -65, column_input = -65;

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
piece *attacking_piece;
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

int is_square_attacked(int x, int y, int side){
	
	int i = 0;
	int j = 0;
	
	//go up file
	for (j = y; j > -1; j--){
		//see if square has a piece
		if (board[j][x] != NULL){
			if (board[j][x]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (board[j][x]->name == ROOK || board[j][x]->name == QUEEN)
					return 1;
			}
		}		
	}
	//go up right
	for (i = 1; x+i<8 && y-i>-1; i++){ //go diag up right
		if (board[y-i][x+i] != NULL){
			if (board[y-i][x+i]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (i == 1 && side == -1){
					if (board[y-i][x+i]->name == PAWN)
						return 1;
				}
				if (board[y-i][x+i]->name == BISHOP || board[y-i][x+i]->name == QUEEN)
					return 1;
			}
		}	
	}
	//go right side
	for (i = x; i < 8; i++){
		//see if square has a piece
		if (board[y][i] != NULL){
			if (board[y][i]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (board[y][i]->name == ROOK || board[y][i]->name == QUEEN)
					return 1;
			}
		}		
	}
	//go down right
	for (i = 1; x+i<8 && y+i<8; i++){ //go diag down right
		if (board[y+i][x+i] != NULL){
			if (board[y+i][x+i]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (i == 1 && side == 1){
					if (board[y+i][x+i]->name == PAWN)
						return 1;
				}
				if (board[y+i][x+i]->name == BISHOP || board[y+i][x+i]->name == QUEEN)
					return 1;
			}
		}	
	}
	//go down
	for (j = y; j < 8; j++){
		//see if square has a piece
		if (board[j][x] != NULL){
			if (board[j][x]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (board[j][x]->name == ROOK || board[j][x]->name == QUEEN)
					return 1;
			}
		}		
	}
	//go down left
	for (i = 1; x-i<8 && y+i<8; i++){ //go diag down left
		if (board[y+i][x-i] != NULL){
			if (board[y+i][x-i]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (i == 1 && side == 1){
					if (board[y+i][x-i]->name == PAWN)
						return 1;
				}
				if (board[y+i][x-i]->name == BISHOP || board[y+i][x-i]->name == QUEEN)
					return 1;
			}
		}	
	}
	//go left
	for (i = x; i > -1; i--){
		//see if square has a piece
		if (board[y][i] != NULL){
			if (board[y][i]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (board[y][i]->name == ROOK || board[y][i]->name == QUEEN)
					return 1;
			}
		}		
	}
	//go up left
	for (i = 1; x-i<8 && y-i<8; i++){ //go diag down left
		if (board[y-i][x-i] != NULL){
			if (board[y-i][x-i]->side == side) //one of our piece. it's blocking square. break
				break;
			else{ //it's enemy piece. check piece
				if (i == 1 && side == -1){
					if (board[y-i][x-i]->name == PAWN)
						return 1;
				}
				if (board[y-i][x-i]->name == BISHOP || board[y+i][x-i]->name == QUEEN)
					return 1;
			}
		}	
	}
	//knight moves
	//big up right
	if ((((y-2) > -1) && ((x+1)<8)) && (board[y-2][x+1]->side != side) && board[y-2][x+1]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
		
	//small up right
	if ((((y-1) > -1) && ((x+2)<8)) && (board[y-1][x+2]->side != side) && board[y-1][x+2]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
	
    //big up left
	if ((((y-2) > -1) && ((x-1)>-1)) && (board[y-2][x-1]->side != side) && board[y-2][x-1]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
	
	//small up left
	if ((((y-1) > -1) && ((x-2)>-1)) && (board[y-1][x-2]->side != side) && board[y-1][x-2]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
	
    //big down right
	if ((((y+2) < 8) && ((x+1)<8)) && (board[y+2][x+1]->side != side) && board[y+2][x+1]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
    
	//small down right
	if ((((y+1) < 8) && ((x+2)<8)) && (board[y+1][x+2]->side != side) && board[y+1][x+2]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
	
	//big down left
	if ((((y+2) < 8) && ((x-1)>-1)) && (board[y+2][x-1]->side != side) && board[y+2][x-1]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
		
	//small down left
	if ((((y+1) < 8) && ((x-2)>-1)) && (board[y+1][x-2]->side != side) && board[y+1][x-2]->name == KNIGHT) //make sure no piece there or it's enemy piece
		return 1;
	
	
	return 0;
	
}

int find_blocker(int x, int y, int side){
	
	int i = 0;
	int j = 0;
	
	//go up file
	for (j = y; j > -1; j--){
		//see if square has a piece
		if (board[j][x] != NULL){
			if (board[j][x]->side == side){ //one of our piece. it can save the day
				if (board[j][x]->name == ROOK || board[j][x]->name == QUEEN) {
					avail[j][x] = 1;
				}
			}
			break;
		}		
	}
	//go up right
	for (i = 1; x+i<8 && y-i>-1; i++){ //go diag up right
		if (board[y-i][x+i] != NULL){
			if (board[y-i][x+i]->side == side){ //one of our piece. it can save the day
					if (board[y-i][x+i]->name == BISHOP || board[y-i][x+i]->name == QUEEN){
						avail[y-i][x+i] = 1;
					}
			}
			break;
		}	
	}
	//go right side
	for (i = x; i < 8; i++){
		//see if square has a piece
		if (board[y][i] != NULL){
			if (board[y][i]->side == side){ //one of our piece. it can save the day
				if (board[y][i]->name == ROOK || board[y][i]->name == QUEEN){
					avail[y-i][x+i] = 1;
				}
			}
			break;
		}		
	}
	//go down right
	for (i = 1; x+i<8 && y+i<8; i++){ //go diag down right
		if (board[y+i][x+i] != NULL){
			if (board[y+i][x+i]->side == side){ //one of our piece. it can save the day
				if (board[y+i][x+i]->name == BISHOP || board[y+i][x+i]->name == QUEEN){
					avail[y+i][x+i] = 1;
				}
			}
			break;
		}	
	}
	//go down
	for (j = y; j < 8; j++){
		//see if square has a piece
		if (board[j][x] != NULL){
			if (board[j][x]->side == side){ //one of our piece. it can save the day
				if (board[j][x]->name == ROOK || board[j][x]->name == QUEEN){
					avail[j][x] = 1;
				}
			}
			break;
		}		
	}
	//go down left
	for (i = 1; x-i<8 && y+i<8; i++){ //go diag down left
		if (board[y+i][x-i] != NULL){
			if (board[y+i][x-i]->side == side){ //one of our piece. it can save the day
				if (board[y+i][x-i]->name == BISHOP || board[y+i][x-i]->name == QUEEN){
					avail[y+i][x-i] = 1;
				}
			}
			break;
		}	
	}
	//go left
	for (i = x; i > -1; i--){
		//see if square has a piece
		if (board[y][i] != NULL){
			if (board[y][i]->side == side){ //one of our piece. it can save the day
				if (board[y][i]->name == ROOK || board[y][i]->name == QUEEN){
					avail[y][i] = 1;
				}
			}
			break;
		}		
	}
	//go up left
	for (i = 1; x-i<8 && y-i<8; i++){ //go diag down left
		if (board[y-i][x-i] != NULL){
			if (board[y-i][x-i]->side == side){ //one of our piece. it can save the day
				if (board[y-i][x-i]->name == BISHOP || board[y+i][x-i]->name == QUEEN){
					avail[y-i][x-i] = 1;
				}
			}
			break;
		}	
	}
	//knight moves
	//big up right
	if ((((y-2) > -1) && ((x+1)<8)) && (board[y-2][x+1]->side == side) && board[y-2][x+1]->name == KNIGHT) //knight can block
		avail[y-2][x+1] = 1;
		
	//small up right
	if ((((y-1) > -1) && ((x+2)<8)) && (board[y-1][x+2]->side != side) && board[y-1][x+2]->name == KNIGHT) //knight can block
		avail[y-1][x+2] = 1;
	
    //big up left
	if ((((y-2) > -1) && ((x-1)>-1)) && (board[y-2][x-1]->side != side) && board[y-2][x-1]->name == KNIGHT) //knight can block
		avail[y-2][x-1] = 1;
	
	//small up left
	if ((((y-1) > -1) && ((x-2)>-1)) && (board[y-1][x-2]->side != side) && board[y-1][x-2]->name == KNIGHT) //knight can block
		avail[y-1][x-2] = 1;
	
    //big down right
	if ((((y+2) < 8) && ((x+1)<8)) && (board[y+2][x+1]->side != side) && board[y+2][x+1]->name == KNIGHT) //knight can block
		avail[y+2][x+1] = 1;
    
	//small down right
	if ((((y+1) < 8) && ((x+2)<8)) && (board[y+1][x+2]->side != side) && board[y+1][x+2]->name == KNIGHT) //knight can block
		avail[y+1][x+2] = 1;
	
	//big down left
	if ((((y+2) < 8) && ((x-1)>-1)) && (board[y+2][x-1]->side != side) && board[y+2][x-1]->name == KNIGHT) //knight can block
		avail[y+2][x-1] = 1;
		
	//small down left
	if ((((y+1) < 8) && ((x-2)>-1)) && (board[y+1][x-2]->side != side) && board[y+1][x-2]->name == KNIGHT) //knight can block
		avail[y+1][x-2] = 1;

	
	return 0;
}

static PT_THREAD(protothread_endgame(struct pt *pt)){
    PT_BEGIN(pt); 
    PT_WAIT_UNTIL(pt, game_over() != 0);
    tft_fillScreen(ILI9340_BLACK);
    tft_setTextSize(3);
    tft_setCursor(30, 140+SHIFT);
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
    tft_setCursor(50, 240);
    tft_setTextColor(ILI9340_YELLOW);  tft_setTextSize(2);
    tft_writeString("Time Left\n");    
    while(1) {         
        PT_YIELD_TIME_msec(1000);  // yield time 1 second
        if(current_player == 1 && white_time_seconds > 0)
            white_time_seconds--;
        //;
        else if(current_player == -1 && black_time_seconds > 0)
            black_time_seconds--;
        
        tft_fillRect(0, 270, 240, 40, ILI9340_BLACK);// x,y,w,h,color
        tft_setTextSize(4);
        tft_setCursor(20, 270);
        tft_setTextColor(ILI9340_WHITE);      
        sprintf(buffer,"%d", white_time_seconds);
        tft_writeString(buffer);
        tft_setCursor(130, 270);
        tft_setTextColor(ILI9340_RED);
        sprintf(buffer,"%d", black_time_seconds);
        tft_writeString(buffer);        
    } // END WHILE(1)
    PT_END(pt);
}
// === UART Input Thread =================================================
static PT_THREAD (protothread_keyboard(struct pt *pt)){
    PT_BEGIN(pt);
      while(1) { 
        if(is_row_pressed == 1 && is_column_pressed == 1)   {
            if((board[row_input][column_input] == NULL || (board[row_input][column_input]->side != current_player)) && state == IDLE){      
                is_row_pressed = 0;
                is_column_pressed = 0;
                column_input = -65;
            }
            else if(state == IDLE)   {
                init_x = column_input; 
                init_y = row_input; 
                is_row_pressed = 0;
                is_column_pressed = 0;
                column_input = -65;
                state++;
                signal = 1;
            }
            else if (init_x == column_input && init_y == row_input)   {
                is_row_pressed = 0;
                is_column_pressed = 0;
                column_input = -65;
                state = -1;
                signal = 1;
            }
            else if (avail[row_input][column_input] == NULL && state == CALC){
                is_row_pressed = 0;
                is_column_pressed = 0;
                column_input = -65;
            }
            else if (state == CALC) {
                end_x = column_input; 
                end_y = row_input;     
                is_row_pressed = 0;
                is_column_pressed = 0;
                column_input = -65;
                state = 0;
                signal = 1;
                if(current_player == 1) {
                    printf("White: ");
                    printf("%c%1c\t %c%1c\n\r", init_x+97, init_y+49, end_x+97, end_y+49);
                }
                else    {
                    printf("Black: ");
                    printf("%c%1c\t %c%1c\n\n\r", init_x+97, init_y+49, end_x+97, end_y+49);
                }
            }
        }
        PT_YIELD_TIME_msec(500);
        tft_fillRect(30, 30, 140, 10, ILI9340_BLACK);// x,y,w,h,color
        tft_setCursor(30, 30);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1);     
        tft_writeString("Column(letter):");
        sprintf(buffer, "%c", column_input+97);
        tft_writeString(buffer);
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
         
        tft_setCursor(50+20*i, 60+SHIFT);
        sprintf(buffer, "%i", i+1);
        tft_writeString(buffer);
        tft_setCursor(50+20*i, 240+SHIFT);
        sprintf(buffer, "%i", i+1);
        tft_writeString(buffer);
        
        tft_setCursor(30, (80+20*i)+SHIFT);
        sprintf(buffer, "%c", 65+i);
        tft_writeString(buffer);
        tft_setCursor(210, (80+20*i)+SHIFT);
        sprintf(buffer, "%c", 65+i);
        tft_writeString(buffer);
    }
    //updates the chess board each iteration
    while(1) {
        PT_YIELD_TIME_msec(500);
        tft_fillRect(50, 80+SHIFT, 160, 160, ILI9340_BLACK);// x,y,w,h,color
        for(j = 0; j < 8; j++ ) {
            for(i = 0; i < 8; i++)  {
                tft_setCursor(50+20*j, (80+20*i)+SHIFT);
                tft_setTextSize(2);                  
                if(board[j][i] == NULL) {                       
                    tft_setTextColor(ILI9340_BLUE); 
                    tft_writeString("|");
                }
                else    {
                    if(board[j][i]->side == 1)
                        tft_setTextColor(ILI9340_WHITE);
                    else
                        tft_setTextColor(ILI9340_RED);
                    if (board[j][i]->name == PAWN) 
                        tft_writeString("P");
                    else if (board[j][i]->name == ROOK) 
                        tft_writeString("R");
                    else if (board[j][i]->name == KNIGHT) 
                        tft_writeString("H");
                    else if (board[j][i]->name == BISHOP) 
                        tft_writeString("B");
                    else if (board[j][i]->name == QUEEN) 
                        tft_writeString("Q");
                    else if (board[j][i]->name == KING) 
                        tft_writeString("K");
                }
                //prints out * on the pieces able to be selected depending on the players turn
                if(state == IDLE && board[j][i] != NULL && board[j][i]->side == current_player)    {
                    tft_setCursor(53+20*j, (80+20*i)+SHIFT);                                       
                    tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1);
                    tft_writeString("*");
                }
                //print out * on the places able to move with that piece
                else if(state == CALC && avail[j][i] == 1)   {    
                    tft_setCursor(53+20*j, (80+20*i)+SHIFT);                                       
                    tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(1);
                    tft_writeString("*");
                }
            }
        }
    } // END WHILE(1)
  PT_END(pt);
} // timer thread
// === Calculate Move and Make Move Thread ==================================
static PT_THREAD (protothread_move(struct pt *pt)){
    PT_BEGIN(pt);
        while(1) {       
          // if button or command is entered for both start and end, make the move
            PT_YIELD_UNTIL(pt, signal);
            if(state == RESET)
                state++;
            else if(state == IDLE) 
                move_piece();
            else if(state == CALC)
                calc_piece_moves();
            signal = 0;
        } // END WHILE(1)
	PT_END(pt);
}
// === Button Press =============================================
static PT_THREAD (protothread_button(struct pt *pt)){   
    PT_BEGIN(pt);  
/*===================Pin Layout=========================
 Pin RA1, RB0-RB2 used by TFT screen
 Column Input - RB4 
 CA           - RB7
 CB           - RB8
 CC           - RB9
  
 Row Input    - RA4
 RA           - RA0
 RB           - RA2
 RC           - RA3
 =======================================================*/
    mPORTASetPinsDigitalIn(BIT_4);                  //Set port as input
    mPORTBSetPinsDigitalIn(BIT_4|BIT_13|BIT_14);    //Set port as input
    mPORTBSetPinsDigitalOut(BIT_7|BIT_8|BIT_9);
    mPORTASetPinsDigitalOut(BIT_0|BIT_2|BIT_3);
    
        while(1) {       
          
          mPORTBClearBits(BIT_7|BIT_8|BIT_9);
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_4) != 0)    {
              row_input = 0;
              is_row_pressed = 1;
          }
          
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_13) != 0)   {
              row_input = 5;
              is_row_pressed = 1;
          }
          
          mPORTBSetBits(BIT_7);
          mPORTBClearBits(BIT_8|BIT_9);
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_4) != 0)    {
              row_input = 1;
              is_row_pressed = 1;
          }
          
          mPORTBSetBits(BIT_8);
          mPORTBClearBits(BIT_7|BIT_9);
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_4) != 0)    {
              row_input = 2;
              is_row_pressed = 1;
          }
          
          mPORTBSetBits(BIT_7|BIT_8);
          mPORTBClearBits(BIT_9);
          PT_YIELD_TIME_msec(5);
          if(mPORTBReadBits(BIT_4) != 0)    {
              row_input = 3;
              is_row_pressed = 1;
          }
          
          mPORTBSetBits(BIT_9);
          mPORTBClearBits(BIT_7|BIT_8);
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_4) != 0)    {
              row_input = 4;
              is_row_pressed = 1;
          }
      
          mPORTBSetBits(BIT_8|BIT_9);
          mPORTBClearBits(BIT_7);
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_4) != 0)    {
              row_input = 6;
              is_row_pressed = 1;
          }
          
          mPORTBSetBits(BIT_7|BIT_8|BIT_9);
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_4) != 0)    {
              row_input = 7;
              is_row_pressed = 1;
          }
              
          
          mPORTAClearBits(BIT_0|BIT_2|BIT_3);
          PT_YIELD_TIME_msec(1);
          if(mPORTAReadBits(BIT_4) != 0)    {
              column_input = 0;
              is_column_pressed = 1;
          }
          
          PT_YIELD_TIME_msec(1);
          if(mPORTBReadBits(BIT_14) != 0)   {
              column_input = 5;
              is_column_pressed = 1;
          }
          
          mPORTASetBits(BIT_0);
          mPORTAClearBits(BIT_2|BIT_3);
          PT_YIELD_TIME_msec(1);
          if(mPORTAReadBits(BIT_4) != 0)    {
              column_input = 1;
              is_column_pressed = 1;
          }
          
          mPORTASetBits(BIT_2);
          mPORTAClearBits(BIT_0|BIT_3);
          PT_YIELD_TIME_msec(1);
          if(mPORTAReadBits(BIT_4) != 0)    {
              column_input = 2;
              is_column_pressed = 1;
          }
          
          mPORTASetBits(BIT_0|BIT_2);
          mPORTAClearBits(BIT_3);
          PT_YIELD_TIME_msec(1);
          if(mPORTAReadBits(BIT_4) != 0)    {
              column_input = 3;
              is_column_pressed = 1;
          }
          
          mPORTASetBits(BIT_3);
          mPORTAClearBits(BIT_0|BIT_2);
          PT_YIELD_TIME_msec(1);
          if(mPORTAReadBits(BIT_4) != 0)    {
              column_input = 4;
              is_column_pressed = 1;
          }
          
          mPORTASetBits(BIT_2|BIT_3);
          mPORTAClearBits(BIT_0);
          PT_YIELD_TIME_msec(1);
          if(mPORTAReadBits(BIT_4) != 0)    {
              column_input = 6;
              is_column_pressed = 1;
          }
          
          mPORTASetBits(BIT_0|BIT_2|BIT_3);
          PT_YIELD_TIME_msec(1);
          if(mPORTAReadBits(BIT_4) != 0)    {
              column_input = 7;
              is_column_pressed = 1;
          }
        PT_YIELD_TIME_msec(5);
          
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
  PT_INIT(&pt_time);
  PT_INIT(&pt_out);
  PT_INIT(&pt_key);
  PT_INIT(&pt_move);
  PT_INIT(&pt_end);
  PT_INIT(&pt_but);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  tft_setRotation(2);
  initialize_board();

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_time));
      PT_SCHEDULE(protothread_keyboard(&pt_key));
      PT_SCHEDULE(protothread_output(&pt_out));
      PT_SCHEDULE(protothread_move(&pt_move));
      PT_SCHEDULE(protothread_endgame(&pt_end));
	  PT_SCHEDULE(protothread_button(&pt_but));
  }
 } // main
// === end  ======================================================
