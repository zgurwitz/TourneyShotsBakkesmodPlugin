#include "pch.h"
#include "TourneyShots.h"


//TODO: Variable Resolution
BAKKESMOD_PLUGIN(TourneyShots, "Display shots during tournaments", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

//variables that need to last throughout the program
bool tied = false;
bool cVarOnlyIfTied = true;
bool cVarOnlyIfTournament = true;
int shotsPlayer = 0;
int shotsEnemy = 0;



void TourneyShots::onLoad()
{
	_globalCvarManager = cvarManager;
	
	//Initialize user-controlled variables, both default to true
	cvarManager->registerCvar("tourney_only_if_tied", "1", "Only display shots if the game is tied", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		cVarOnlyIfTied = cvar.getBoolValue();
			});
	cvarManager->registerCvar("tourney_only_if_tournament", "1", "Only display shots in a tournament", true, true, 0, true, 1)
		.addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
		cVarOnlyIfTournament = cvar.getBoolValue();
			});
	
	
	this->Log("Tourney Shots loaded succesfully");
	this->LoadHooks();
	
	
}

void TourneyShots::onUnload()
{
}

void TourneyShots::LoadHooks()
{
	//check if the game is tied when the game starts and at the start of each goal replay
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&TourneyShots::GameTied, this));
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState", std::bind(&TourneyShots::GameTied, this));
	//reset the variables when a game ends
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&TourneyShots::Reset, this));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&TourneyShots::Reset, this));
	//handle ticker to detect shots
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			onStatTickerMessage(params);
		});
	//draw the text
	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		 Render(canvas); 
		});

}

//Makes Logging to the console easier
void TourneyShots::Log(std::string msg) 
{
	cvarManager->log(msg);
}

//Check if the game is tied by getting the total score and the player team score,
//and then seeing if the first is double the second.
void TourneyShots::GameTied()
{
	//Make sure you're in a game
	if (!gameWrapper->IsInOnlineGame() || gameWrapper->IsInReplay()) { Log("Not in a game.");  Reset();  return; }
	
	//Get server
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) { Log("Null server"); return; }
	
	/*
	 *get playlist IDs
	 *GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	 *if (!playlist) return;
	 *int playlistID = playlist.GetPlaylistId();
	 *Log(std::to_string(playlistID));
 	 */

	//Get total score
	int tot = server.GetTotalScore();
	
	//Get player
	PlayerControllerWrapper playerController = gameWrapper->GetPlayerController();
	if (!playerController) { Log("Null controller"); return; }

	//Get playerPRI
	PriWrapper playerPRI = playerController.GetPRI();
	if (!playerPRI) { Log("Null player PRI"); return; }
	
	//Get the player's team
	TeamInfoWrapper playerTeam = playerPRI.GetTeam();
	if (!playerTeam) { Log("Null player team"); return; }

	//Get the player's team's score
	int score = playerTeam.GetScore();

	//check if tied, set tied variable appropriately
	if (2 * score == tot) {
		tied = true;
		Log("Tie game!");
	}
	else {
		tied = false;
		Log("Not tied.");
	}
}

//Draw the shot stat to the screen, only is in the right place for [MY RESOLUTION]
void TourneyShots::Render(CanvasWrapper canvas) {
	//Make sure you're in a game
	if (!gameWrapper->IsInOnlineGame() || gameWrapper->IsInReplay()) { return; }
	
	//Get server
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) { Log("Null server"); return; }
	
	//get the playlist ID, 34 = Tournament
	GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	if (!playlist) { Log("Null playlist");  return; }
	int playlistID = playlist.GetPlaylistId();

	//if the game is tied, or in a tournament, or the user has decided to relax these restrictions
	if ((tied  || !cVarOnlyIfTied  )  && (playlistID == 34 || !cVarOnlyIfTournament)) {
		// defines color of text in RGBA 0-255
		LinearColor colors;
		//color for box
		colors.R = 100;
		colors.G = 100;
		colors.B = 100;
		colors.A = 200;
		canvas.SetColor(colors);
		//positions the box on the screen 
		canvas.SetPosition(Vector2F{ 800.0, 100.0 });
		
		//put a plus on the shot differential if it's positive
		std::string plus = "";
		if (shotsPlayer - shotsEnemy > 0) { plus += "+"; }
		std::string toPrint = std::to_string(shotsPlayer) + "             " + plus + std::to_string(shotsPlayer - shotsEnemy)
			+ "               " + std::to_string(shotsEnemy);
		
		//draw the box behind the score for visibility
		canvas.FillBox(Vector2F{canvas.GetStringSize(toPrint,2.0,2.0)});
		
		//color for text
		colors.R = 255;
		colors.G = 255;
		colors.B = 0;
		colors.A = 255;
		canvas.SetColor(colors);

		//position the text on the screen
		canvas.SetPosition(Vector2F{ 800.0, 100.0 });
		//Draw the actual text, player team shots on the left, enemy shots on the right, differential in the middle
		canvas.DrawString(toPrint, 2.0, 2.0, false);
	}
}



//Add the shots to the right variable when a shot happens
//edited from code by ubelhj at https://bakkesmodwiki.github.io/functions/stat_events/

void TourneyShots::onStatTickerMessage(void* params) {
	
	//Check if in game
	if (!gameWrapper->IsInOnlineGame() || gameWrapper->IsInReplay()) { Log("Not in a game.");  Reset();  return; }
	
	//Variables that are needed for reading the stat ticker
	struct StatTickerParams {
		uintptr_t Receiver;
		uintptr_t Victim;
		uintptr_t StatEvent;
	}; 
	StatTickerParams* pStruct = (StatTickerParams*)params;
	PriWrapper receiver = PriWrapper(pStruct->Receiver);
	PriWrapper victim = PriWrapper(pStruct->Victim);
	StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);
	if (!statEvent) { Log("Null statEvent"); return; }

	//only care if it's a shot
	if (statEvent.GetEventName() == "Shot") {
		//null check the shooter and their team
		if (!receiver) { Log("Null shooter PRI"); return; }
		TeamInfoWrapper shooterTeam = receiver.GetTeam();
		if (!shooterTeam) { Log("Null shooter team"); return; }

		//get the user's team to compare to the reciever
		PlayerControllerWrapper playerController = gameWrapper->GetPlayerController();
		if (!playerController) { Log("Null controller"); return; }
		PriWrapper playerPRI = playerController.GetPRI();
		if (!playerPRI) { Log("Null player PRI"); return; }
		TeamInfoWrapper playerTeam = playerPRI.GetTeam();
		if (!playerTeam) { Log("Null player team"); return; }
		int primaryTeamIndex = playerTeam.GetTeamIndex();

		//add the shot taken to the correct team
		if (shooterTeam.GetTeamIndex() == primaryTeamIndex) {
			shotsPlayer++;
		}
		else {
			shotsEnemy++;
		}
	}
}

//Reset shot total
void TourneyShots::Reset() {
	shotsEnemy = 0;
	shotsPlayer = 0;
	tied = false;
}