#ifndef NOVA_ASSAULT_GAME_VIEW_H
#define NOVA_ASSAULT_GAME_VIEW_H

#include <View.h>

class BBitmap;

const int32 kEnemyCount = 14;
const int32 kBulletCount = 48;
const int32 kItemCount = 4;

struct Bullet {
	float x, y, dy;
	bool enemy;
	bool active;
};

struct Enemy {
	float x, y, baseX, phase;
	int32 hp;
	bool alive;
};

struct Item {
	float x, y;
	int32 type;
	bool active;
};

class GameView : public BView {
public:
	GameView();
	~GameView();
	void AttachedToWindow() override;
	void Draw(BRect update) override;
	void KeyDown(const char* bytes, int32 count) override;
	void MouseDown(BPoint point) override;
	void MessageReceived(BMessage* message) override;

private:
	void ResetGame();
	void StartWave();
	void UpdateGame();
	void FirePlayer();
	void FireEnemy(Enemy& enemy);
	void SpawnItem(float x, float y);
	void DrawBackground(BRect bounds);
	void DrawHud(BRect bounds);
	void DrawPlayer(BRect bounds);
	void DrawEnemy(const Enemy& enemy);
	void DrawBullets();
	void DrawItems();
	void DrawGameOver(BRect bounds);
	bool Hits(float ax, float ay, float ar, float bx, float by, float br) const;

	Enemy fEnemies[kEnemyCount];
	Bullet fBullets[kBulletCount];
	Item fItems[kItemCount];
	float fPlayerX;
	float fTargetX;
	int32 fScore;
	int32 fLives;
	int32 fWave;
	int32 fPower;
	int32 fShieldFrames;
	int32 fFireFrames;
	int32 fSpawnFrames;
	bool fGameOver;
	BBitmap* fShipSprite;
};

#endif
