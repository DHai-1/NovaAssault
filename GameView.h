#ifndef NOVA_ASSAULT_GAME_VIEW_H
#define NOVA_ASSAULT_GAME_VIEW_H

#include <View.h>

class BBitmap;

const int32 kEnemyCount = 18;
const int32 kBulletCount = 140;
const int32 kItemCount = 8;
const int32 kExplosionCount = 28;
const int32 kParticleCount = 220;
const int32 kStarCount = 170;
const int32 kTrailCount = 10;

enum enemy_kind { kEnemySmall = 0, kEnemyMedium = 1, kEnemyBig = 2 };
enum item_type { kItemPower = 0, kItemShield = 1, kItemFan = 2 };

struct Bullet {
	float x, y, dx, dy;
	bool enemy;
	bool active;
};

struct Enemy {
	float x, y, baseX, phase;
	int32 hp;
	int32 kind;
	int32 flash;
	int32 entry;
	int32 fireDelay;
	bool alive;
};

struct Boss {
	float x, y, baseX, phase;
	int32 hp, maxHp;
	int32 flash, fireTimer, entry, volley;
	bool alive;
};

struct Item {
	float x, y;
	int32 type;
	int32 phase;
	bool active;
};

struct Explosion {
	float x, y;
	int32 frame;
	int32 size;
	bool active;
};

struct Particle {
	float x, y, vx, vy;
	int32 life, maxLife, size, color;
};

struct Star {
	float x, y, speed;
	int32 size, brightness, layer;
};

class GameView : public BView {
public:
								GameView();
	virtual						~GameView();

	virtual void				AttachedToWindow();
	virtual void				Draw(BRect update);
	virtual void				KeyDown(const char* bytes, int32 count);
	virtual void				MouseDown(BPoint point);
	virtual void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* message);
	virtual void				MessageReceived(BMessage* message);

private:
	void						LoadSprites();
	void						InitStars();
	void						ResetGame();
	void						StartWave();
	void						UpdateGame();
	void						UpdateParticles();

	void						AddBullet(float x, float y, float dx,
									float dy, bool enemy);
	void						FirePlayer();
	void						FireEnemy(Enemy& enemy);
	void						FireBoss();
	void						DamageBoss(int32 amount, float x, float y);
	void						SpawnItem(float x, float y,
									int32 forcedType = -1);
	void						SpawnExplosion(float x, float y, int32 size);
	void						SpawnParticles(float x, float y, int32 count,
									int32 color, float speed, int32 size);
	void						Shake(float amount);

	void						DrawBackground(BRect bounds);
	void						DrawHud(BRect bounds);
	void						DrawWaveBanner(BRect bounds);
	void						DrawBoss();
	void						DrawBossHealth(BRect bounds);
	void						DrawPlayer(BRect bounds);
	void						DrawEnemy(const Enemy& enemy, int32 index);
	void						DrawBullets();
	void						DrawItems();
	void						DrawExplosions();
	void						DrawParticles();
	void						DrawGameOver(BRect bounds);

	bool						Hits(float ax, float ay, float ar,
									float bx, float by, float br) const;

	Enemy						fEnemies[kEnemyCount];
	Boss						fBoss;
	Bullet						fBullets[kBulletCount];
	Item						fItems[kItemCount];
	Explosion					fExplosions[kExplosionCount];
	Particle					fParticles[kParticleCount];
	Star						fStars[kStarCount];

	float						fPlayerX;
	float						fTargetX;
	float						fPrevX[kTrailCount];
	int32						fScore;
	int32						fLives;
	int32						fWave;
	int32						fPower;
	int32						fShieldFrames;
	int32						fFanFrames;
	int32						fFireFrames;
	int32						fSpawnFrames;
	int32						fTick;
	int32						fCombo;
	int32						fBestCombo;
	int32						fComboTimer;
	int32						fWaveBanner;
	int32						fGameOverTick;
	int32						fFlash;
	float						fShake;
	float						fShakeX;
	float						fShakeY;
	bool						fGameOver;
	bool						fBossActive;
	bool						fSoundOn;

	BBitmap*					fShip;
	BBitmap*					fEnemySmall;
	BBitmap*					fEnemyMedium;
	BBitmap*					fEnemyBig;
	BBitmap*					fExplosion;
	BBitmap*					fLaser;
	BBitmap*					fPowerUp;
	BBitmap*					fClouds;
};

#endif // NOVA_ASSAULT_GAME_VIEW_H
