
/* The media player control module (mplc) is the connection point
 * between media player instances and media controllers.
 *
 * A media player has (access to) media content and knows how to play
 * it.  A media controller reads or gets information from a player and
 * controls the player by giving it commands and setting parameters.
 * 
 * The media player control module allows media player implementations
 * to make themselves available to controllers.  And it allows
 * controllers to access, and get updates from, any player.
 *
 * The media player control module allows both local and remote
 * control of local player instances: A media controller may be a
 * local application, or it may be a Media Control Service relaying
 * requests from a remote Media Control Client.
 *
 * There may be either local or remote control, or both, or even
 * multiple instances of each.
 *
 * TBDecided: The media control module also allows local control of
 * both local and remote player instances.
 *
 * The media player control module has a "current" player, which is
 * the player to which commands will be applied if no player is
 * specified.  This will also be the player for the GMCS.
 * 
 */

/* Questions:
 * - Should a controller be able to control more than one player
 *   instance? If so, the callbacks to the controller must take a
 *   player id parameter, and the controller must be able to subscribe
 *   to more than one player instance
 *
 * - Use pointers or indexes for the player instances?
 *   VOCS/AICS uses pointers
 *
 * - Are players static, or can they be allowed to come and go?
 *   If static, all players must register before the controllers.
 *   If dynamic, there must be callback to controllers on changes
 *   Assume static for now
 */ 


/* PUBLIC API FOR CONTROLLERS */

/* @brief Get players
 *
 * TBDecicded: Get a struct with info for all player, or get info for
 * one and repeat to get next?
 *
 * TBDecided: Get only player IDs, or also get names (to know which
 * players are interesting)
 */ 
int bt_mplc_ctrl_players_get();

/* Get name of player - to know which player this is
 *
 * (Could also get names as part of players_get)
 * (But there will be a name call anyway.)
 */
char bt_mplc_ctrl_player_name_get(int player);


/* Callbacks from the player controller to the controller */
struct bt_mplc_ctrl_cb {

	/* Called on track position changes */
	bt_mplc_ctrl_track_position_change(int player_id);
};

/* @brief Register a controller
 *
 * @param cb	Callbacks
 * @param id	The player(s) to subscribe to (receive callbacks from)
 */
int bt_mplc_ctrl_register(struct bt_mplc_ctrl_cb cb, uint8_t id);


/* Calls from the controller to the player controller */

/* Set the current player */
int bt_mctrl_set_current_player(int player_id);	

/* Get the track position */
int32_t mpl_track_position_get(int player_id);

/* Set the track position */
void mpl_track_position_set(int player_id, int32_t position);



/* PUBLIC API FOR PLAYERS */

/* Calls from the player controller the player */	
struct bt_mplc_player_calls {

	/* Called to get track position */
	int32_t mpl_track_position_get(void);

	/* Called to set track position */
	void mpl_track_position_set(int32_t position);
};

/* @brief Register a player
 *
 * param[in] calls	The player calls
 * param[out] id	player id after registration
 */
int bt_mctrl_player_register(struct bt_mplc_player_calls *call, int *id);

/* Callbacks from the player to the player controller */

/* Call on track position changes */
void mpl_track_position_change(int player_id, int32_t position);



/* INTERNAL API */

struct controller {
	/* The controller callbacks */
	struct bt_mplc_ctrl_cb cb;

	/* The player(s) the controller is subscribed to */
	int  subscribed_player;
};

struct player {
	/* The player calls */
	struct bt_mctrl_player_call calls;
};

/* The players */
struct player players[PLAYER_COUNT];

/* The current player */
struct player *current_player;

/* The controllers */
struct controller controllers[CONTROLLER_COUNT];



	
