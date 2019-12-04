#include "GameManager.h"
#include "Position.h"

using namespace std;

GameManager::GameManager(Parameters params)
{
	turn = false;
	// if (params.get_priority())
	if (true)
		player = true;
	else
		player = false;
	(void)params;
}

GameManager::~GameManager() {}

bool		GameManager::modify_board(uint32_t new_index, uint8_t stone)
{
	if (board[new_index] != 0)
		return false;
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

void		GameManager::change_turn()
{
	turn = !turn;
}

uint8_t		GameManager::get_turn_color()
{
	return (uint8_t)turn;
}

void		GameManager::change_player(Parameters &params)
{
	// if (params.get_mode() != PlayerVsPlayer && params.get_mode() != EngineVsEngine)
		// player = !player;
	(void)params;
}