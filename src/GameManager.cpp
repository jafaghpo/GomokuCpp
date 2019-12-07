#include "GameManager.h"
#include "Position.h"

using namespace std;

GameManager::GameManager(Parameters params)
{
	turn = false;
	if (params.priority == true)
		player = true;
	else
		player = false;
}

GameManager::~GameManager() {}

bool		GameManager::modify_board(uint32_t new_index, uint8_t stone, bool c4_rule)
{
	if (board[new_index] != 0)
		return false;
	if (c4_rule)
	{
		while (new_index < BOARD_CAPACITY - BOARD_SIZE && board[new_index + 19] == 0)
			new_index += 19;
	}
	board.update(new_index, stone);
	history.push_back(make_tuple(new_index, stone));
	return true;
}

uint32_t	GameManager::get_last_move()
{
	return get<0>(history.back());
}

Board		GameManager::get_board()
{
	return board;
}

bool		GameManager::get_player()
{
	return player;
}

uint8_t		GameManager::get_turn_color()
{
	return (uint8_t)turn;
}

void		GameManager::change_player(Parameters &params)
{
	turn = !turn;
	if (params.mode != PlayerVsPlayer && params.mode != EngineVsEngine)
		player = !player;
	
}