#include "GameManager.h"
#include "Position.h"
#include "UserInterface.h"

#include <SDL.h>
#include <stdio.h>
#include <chrono>

using namespace std;

GameManager::GameManager(Parameters params)
{
	this->params = params;
	player = false;
	if (params.priority == true && params.mode != EngineVsEngine)
		player_mode = Human;
	else
		player_mode = Engine;

	move_value[ BlockedTwo ] = (params.rule == Restricted) ? -300 : 50;
	move_value[ None ] = 0;
	move_value[ FreeTwo ] = 100;
	move_value[ BlockedThree ] = 200;
	move_value[ BlockedFour ] = 500;
	move_value[ FreeThree ] = 750;
	move_value[ FreeFour ] = 1500;
	move_value[ Five ] = 1500;
	move_value[ /* Capture */ 8 ] = 300;

	for (int y = 0; y <= BOARD_SIZE / 2; y++)
	{
		for (int x = 0; x <= BOARD_SIZE / 2; x++)
		{
			auto pos_value = std::min(x, y);
			cell_value[y * BOARD_SIZE + x] = pos_value;
			cell_value[y * BOARD_SIZE + (BOARD_SIZE - 1 - x)] = pos_value;
			cell_value[(BOARD_SIZE - 1 - y) * BOARD_SIZE + (BOARD_SIZE - 1 - x)] = pos_value;
			cell_value[(BOARD_SIZE - 1 - y) * BOARD_SIZE + x] = pos_value;
		}
	}
}

GameManager::~GameManager() {}

size_t		GameManager::get_connect4_index(size_t index)
{
	while (index < BOARD_CAPACITY - BOARD_SIZE && board.get(index + BOARD_SIZE) == Empty)
		index += BOARD_SIZE;
	return index;
}

void		GameManager::change_player_turn()
{
	player = !player;
	if (params.mode != PlayerVsPlayer && params.mode != EngineVsEngine)
		player_mode = 1 - player_mode;
}

int			GameManager::get_last_move()
{
	return history.size() ? (int)history.back().last_move : -1;
}

size_t		GameManager::dumb_algo(board_t grid)
{
	for (size_t i = 0; i < BOARD_CAPACITY; i++)
	{
		if (grid[i] == 0)
			return i;
	}
	return 0;
}

void		GameManager::load_history()
{
	if (history.size() > 0)
		history.pop_back();
	if (history.size() > 0)
		history.pop_back();
	if (history.size() == 0)
	{
		board.clear_cells();
		board.clear_indexes();
	}
	else
	{
		board.update(history.back().board);
		auto capture = history.back().capture;
		board.capture[0] = capture[0];
		board.capture[1] = capture[1];
	}
}

GameStatus		GameManager::is_endgame(int index, uint8_t player)
{
	static int 	dirs_win[4] = {Up + Left, Up, Up + Right, Right};

	if (params.rule == Restricted)
	{
		if (board.capture[player] >= 5)
			return static_cast<GameStatus>(player + 3);
	}
	for (int i = 0; i < 4; i++)
	{
		auto sequence = board.get_sequence(index, player, dirs_win[i]);
		if (sequence.type == Five)
		{
			if (params.rule == Restricted)
			{
				if (board.can_capture_win_sequence(index, player, dirs_win[i]))
					return Playing;
				else
					return static_cast<GameStatus>(player + 1);
			}
			else
				return static_cast<GameStatus>(player + 1);
		}
	}
	if (board.is_draw() == true)
		return Draw;
	return Playing;
}

void		GameManager::print_game_status(GameStatus status)
{
	if (status == Draw)
		printf("Draw !\n");
	else if (status < PlayerOneWinByCapture)
		printf("Player %d won the game by lining up 5 stones !\n", status);
	else
		printf("Player %d won the game by capturing 10 enemy stones !\n", status - 2);
	
}

void		GameManager::run_loop()
{
	UserInterface	ui(params);
	SDL_Event		event;
	bool			quit = false;
	GameStatus 		game_status = Playing;
	bool			end_turn = false;
	Position		stone;

	ui.render();
	while (!quit)
	{
		SDL_WaitEvent(&event);
		while (game_status == 0)
		{
			if (player_mode == Human)
			{
				SDL_WaitEvent(&event);
				if (UNDO_EVENT(event))
				{
					load_history();
					ui.print_board(board.cells, get_last_move());
				}
				if (LEFT_CLICK(event))
				{
					stone = ui.get_user_input(Position(event.button.x, event.button.y));
					if (board.can_place(stone.index(), player, params.rule))
					{
						board.play_move(stone.index(), player, params.rule);
						history.push_back(History { board.cells, stone.index(), board.capture });
						printf("Player %d (human) played at position (%d, %d)\n", player + 1, stone.x, stone.y);
						end_turn = true;
					}
					else
						printf("Warning - cannot add a stone at position (%d, %d)\n", stone.x, stone.y);

					ui.print_board(board.cells, get_last_move());
				}
			}
			else
			{
				SDL_PollEvent(&event);
				auto start = chrono::system_clock::now();
				// Run negamax here
				stone = INDEX_TO_POS(dumb_algo(board.cells));
				auto end = chrono::system_clock::now();
				auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
				if (board.can_place(stone.index(), player, params.rule))
				{
					board.play_move(stone.index(), player, params.rule);
					history.push_back(History { board.cells, stone.index(), board.capture });
					printf("Player %d (engine) played at position (%d, %d) in %lld ms\n",
						player + 1, stone.x, stone.y, duration.count());
					end_turn = true;
				}
				else
					printf("Warning - cannot add a stone at position (%d, %d)\n", stone.x, stone.y);

				ui.print_board(board.cells, get_last_move());
			}
			
			if (end_turn == true)
			{
				if ((game_status = is_endgame(stone.index(), player)))
					print_game_status(game_status);
				change_player_turn();
				end_turn = false;
			}
			
			if (CLOSE_EVENT(event))
			{
				quit = true;
				break;
			}
		}
		if (CLOSE_EVENT(event))
		{
			quit = true;
			printf("Exit requested by user. Exiting now...\n");
		}
	}
	ui.FreeSDL();
}