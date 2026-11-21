#ifndef NOVA_ASSAULT_GAME_VIEW_H
#define NOVA_ASSAULT_GAME_VIEW_H

#include <View.h>

class BBitmap;

const int32 kEnemyCount = 18;
const int32 kBulletCount = 64;
const int32 kItemCount = 6;
const int32 kExplosionCount = 24;
const int32 kStarCount = 140;

enum enemy_kind {
	kEnemySmall = 0,
	kEnemyMedium = 1,
	kEnemyBig = 2
};

enum item_type {
	kItemPower = 0,
	kItemShield = 1
};

struct Bullet {
	float x, y, dy;
	bool enemy;
	bool active;
};

struct Enemy {
	float x, y, baseX, phase;
	int32 hp;
	int32 kind;
	int32 flash;
	bool alive;
};

struct Item {
	float x, y;
	int32 type;
	bool active;
};

struct Explosion {
	float x, y;
	int32 frame;
	int32 size;
	bool active;
};

struct Star {
	float x, y;
	float speed;
	int32 size;
	int32 brightness;
};

class GameView : public BView {
public:
								GameView();
	virtual						~GameView();

	virtual void				AttachedToWindow();
	virtual void				Draw(BRect update);
	virtual void				KeyDown(const char* bytes, int32 count);
	virtual void				MouseDown(BPoint point);
	virtual void				MessageReceived(BMessage* message);

private:
	void						LoadSprites();
	void						InitStars();
	void						ResetGame();
	void						StartWave();
	void						UpdateGame();

	void						FirePlayer();
	void						FireEnemy(Enemy& enemy);
	void						SpawnItem(float x, float y);
	void						SpawnExplosion(float x, float y, int32 size);

	void						DrawBackground(BRect bounds);
	void						DrawHud(BRect bounds);
	void						DrawPlayer(BRect bounds);
	void						DrawEnemy(const Enemy& enemy, int32 index);
	void						DrawBullets();
	void						DrawItems();
	void						DrawExplosions();
	void						DrawGameOver(BRect bounds);

	bool						Hits(float ax, float ay, float ar,
									float bx, float by, float br) const;

	Enemy						fEnemies[kEnemyCount];
	Bullet						fBullets[kBulletCount];
	Item						fItems[kItemCount];
	Explosion					fExplosions[kExplosionCount];
	Star						fStars[kStarCount];

	float						fPlayerX;
	float						fTargetX;
	int32						fScore;
	int32						fLives;
	int32						fWave;
	int32						fPower;
	int32						fShieldFrames;
	int32						fFireFrames;
	int32						fSpawnFrames;
	int32						fTick;
	bool						fGameOver;

	BBitmap*					fShip;
	BBitmap*					fEnemySmall;
	BBitmap*					fEnemyMedium;
	BBitmap*					fEnemyBig;
	BBitmap*					fExplosion;
	BBitmap*					fLaser;
	BBitmap*					fPowerUp;
};

#endif // NOVA_ASSAULT_GAME_VIEW_H
