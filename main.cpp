#include <Application.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Window.h>

#include "GameView.h"

enum { MSG_FRAME = 'fram' };

class GameWindow : public BWindow {
public:
	GameWindow()
		: BWindow(BRect(90, 80, 850, 680), "Nova Assault",
			B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
	{
		fGame = new GameView();
		AddChild(fGame);
		fRunner = new BMessageRunner(BMessenger(fGame), new BMessage(MSG_FRAME), 30000);
	}
	~GameWindow() { delete fRunner; }
private:
	GameView* fGame;
	BMessageRunner* fRunner;
};

class NovaAssaultApp : public BApplication {
public:
	NovaAssaultApp() : BApplication("application/x-vnd.dhai-novaassault-v2") {}
	void ReadyToRun() override { (new GameWindow())->Show(); }
};

int main()
{
	NovaAssaultApp app;
	app.Run();
	return 0;
}
