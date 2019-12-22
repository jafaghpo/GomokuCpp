#include "GameManager.h"
#include "Position.h"
#include "UserInterface.h"
#include "Engine.h"

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
		player_mode = AI;
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
		player_mode = player_mode ^ 1;
}

int			GameManager::get_last_move()
{
	return history.size() ? (int)history.back().last_move : -1;
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
		player = false;
	}
	else
	{
		board.update(history.back().board);
		auto capture = history.back().capture;
		board.capture[0] = capture[0];
		board.capture[1] = capture[1];
	}
}

void		GameManager::print_game_status(uint8_t status)
{
	if (status == Draw)
		printf("Draw !\n");
	else if (status < CaptureWin)
		printf("Player %d won the game by lining up 5 stones !\n", status - SequenceWin + 1);
	else
		printf("Player %d won the game by capturing 10 enemy stones !\n", status - CaptureWin + 1);
	
}

void		GameManager::run_loop()
{
	UserInterface	ui(params);
	SDL_Event		event;
	Position		stone;
	Engine			engine(params.rule);
	uint8_t 		game_status = Playing;
	bool			quit = false;
	bool			end_turn = false;

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
						// printf("evaluation of board: %d\n", engine.evaluate_board(board, player));
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
				auto index = engine.get_best_move(board, player, NegamaxWithPruning);
				stone = INDEX_TO_POS(index);
				auto end = chrono::system_clock::now();
				auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

				if (board.can_place(index, player, params.rule))
				{
					board.play_move(index, player, params.rule);
					history.push_back(History { board.cells, index, board.capture });
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
				if ((game_status = board.is_endgame(stone.index(), player, params.rule == Restricted)))
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