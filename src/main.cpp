#define OLC_PGE_APPLICATION

#include <iostream>
#include <math.h>
#include <list>
#include <time.h>
#include "olcPixelGameEngine.h"

const float gameSpeed = 300.0f;

class Pipe {
public:
	float x;
	int y, gap;
	olc::Decal* texture;
	bool givenPoint;


	Pipe(float set_x, olc::Decal* set_texture) {
		texture = set_texture;

		givenPoint = false;

		x = set_x;

		gap = 50  * (float)rand() / (float)RAND_MAX + 220;
		y   = 550 * (float)rand() / (float)RAND_MAX + 150;
	}
	void Update(float elapsedTime) {
		x -= gameSpeed * elapsedTime;
	}
	bool OffScreen() {
		return x + texture->sprite->width < 0;
	}
	void Draw(olc::PixelGameEngine* pge) {
		pge->DrawDecal(olc::vi2d{ (int)x, pge->ScreenHeight() - y }, texture);
		
		pge->DrawRotatedDecal(olc::vi2d{ (int)x, pge->ScreenHeight() - y - gap }, texture, 3.14159f, olc::vf2d{ float(texture->sprite->width), 0.0f });
	}
};

class Bird {
public:
	float pos_y, vel_y;
	const int x = 200;
	const float size = 0.1f;
	olc::Decal* texture;

public:
	void Config(olc::Decal* set_texture, int y) {
		texture = set_texture;
		pos_y = (float)y;
		vel_y = 0.0f;
	}

	void Reset(int y) {
		pos_y = (float)y;
		vel_y = 0.0f;
	}

	void Fall(float elapsedTime) {
		// g = 850
		vel_y += 850.0f * elapsedTime;
		pos_y += vel_y * elapsedTime;
	}
	void Jump(float elapsedTime) {
		vel_y  = -500.0f;
		pos_y += vel_y * elapsedTime;
	}

	bool Collide(olc::PixelGameEngine* pge, Pipe* pipe) {
		if (x + (texture->sprite->width * size / 2.0f) > pipe->x)
			if (x - (texture->sprite->width * size / 2.0f) < pipe->x + (pipe->texture->sprite->width >> 1))
				return (pos_y < pge->ScreenHeight() - pipe->y - pipe->gap) || (pos_y > pge->ScreenHeight() - pipe->y);
		return false;
	}

	bool FloorCollide(int floor) {
		return pos_y + texture->sprite->height * size / 2.0f >= floor;
	}

	void Draw(olc::PixelGameEngine* pge) {
		//pge->DrawDecal(olc::vi2d{ 200, (int)pos_y }, texture, olc::vf2d{ 0.1,0.1 });
		pge->DrawRotatedDecal(olc::vi2d{ x, (int)pos_y }, texture, 0.7f*atanf(0.01f*vel_y),
			olc::vf2d{ float(texture->sprite->width >> 1) , float(texture->sprite->height >> 1)}, olc::vf2d{ size,size });
	}
};


class FlappyBird : public olc::PixelGameEngine
{
public:
	FlappyBird() {
		sAppName = "Flappy Bird";
	}

private:
	olc::Sprite sBird, sBG, sFloor, sPipe;
	int FloorScaling, score, maxScore;
	float FloorMove, BGMove;
	bool endGame, startGame;

	const int birdStartHeight = 400;

	Bird bird;

	olc::Decal* dBG, *dFloor, *dPipe;
	std::list<Pipe*> Pipes;

	void ResetGame() {
		bird.Reset(birdStartHeight);

		if (!Pipes.empty()) {
			for (Pipe* p : Pipes)
				delete p;
		}

		Pipes.clear();

		FloorMove = 0.0f;
		BGMove = 0.0f;
		score = 0;

		endGame = false;
		startGame = true;
	}
protected:
	bool OnUserCreate() override {
		srand(time(NULL));

		sBird.LoadFromFile("Sprites/Bird.png");
		sBG.LoadFromFile("Sprites/BG.png");
		sPipe.LoadFromFile("Sprites/Pipe.png");
		sFloor.LoadFromFile("Sprites/Floor.png");
		FloorScaling = 2;

		bird.Config(new olc::Decal(&sBird), birdStartHeight);

		dBG    = new olc::Decal(&sBG);
		dFloor = new olc::Decal(&sFloor);
		dPipe  = new olc::Decal(&sPipe);

		ResetGame();

		maxScore = 0;
		return true;
	}

	bool OnUserUpdate(float elapsedTime) override {
		if (GetKey(olc::ESCAPE).bPressed) return false;

		Clear(sBG.Sample(0, 0));

		// Controls
		if (!endGame) {
			if (GetKey(olc::SPACE).bPressed) {
				bird.Jump(elapsedTime);
				if (startGame) {
					Pipes.push_back(new Pipe(ScreenWidth(), dPipe));
					Pipes.push_back(new Pipe(ScreenWidth()*1.5f, dPipe));
					startGame = false;
				}
			}
			else if (!startGame)
				bird.Fall(elapsedTime);
		}
		// Update
		if (!endGame) {

			// Update Pipes
			for (Pipe* p : Pipes)
				p->Update(elapsedTime);

			// Update Floor
			FloorMove -= gameSpeed * elapsedTime;
			while (FloorMove < -sFloor.width) FloorMove += sFloor.width;

			// Update BG
			BGMove -= gameSpeed * elapsedTime / 8.0f;
			while (BGMove < -sBG.width) BGMove += sBG.width;
		}

		// Check if Pipes are off screen
		if (!Pipes.empty())
			if (Pipes.front()->OffScreen()) {
				delete Pipes.front();
				Pipes.pop_front();

				Pipes.push_back(new Pipe(ScreenWidth(), dPipe));
			}
		

		// check score
		for (Pipe* p : Pipes) {
			if (!p->givenPoint && p->x < bird.x) {
				p->givenPoint = true;
				score++;
				if (score > maxScore) maxScore = score;
			}
		}
		// Check Death
		if (bird.FloorCollide(ScreenHeight() - FloorScaling * sFloor.height)) {
			endGame = true;
			goto Draw;
		}
		for (Pipe* p : Pipes)
			if (bird.Collide(this, p)) {
				endGame = true;
				goto Draw;
			}
	Draw:

		// Draw BG
		for (int shifted = (int)BGMove; shifted < ScreenWidth(); shifted += sBG.width)
			DrawDecal(olc::vi2d{ shifted, ScreenHeight() - FloorScaling * sFloor.height - sBG.height }, dBG);

		
		// Draw Pipes
		for (Pipe* p : Pipes)
			p->Draw(this);

		
		// Draw Floor
		for (int shifted = (int)FloorMove; shifted < ScreenWidth(); shifted += sFloor.width)
			DrawDecal(olc::vi2d{ shifted, ScreenHeight() - FloorScaling * sFloor.height }, dFloor, olc::vf2d{ (float)FloorScaling, (float)FloorScaling });


		// Draw Bird
		bird.Draw(this);

		// Draw Score
		DrawStringDecal(olc::vi2d{ 10, 10 }, "Score: " + std::to_string(score), olc::WHITE, olc::vf2d{4.0f, 4.0f});

		// Draw Max Score
		DrawStringDecal(olc::vi2d{ 10, 50 }, "Max Score: " + std::to_string(maxScore), olc::WHITE, olc::vf2d{4.0f, 4.0f});

		if (endGame) {
			FillRectDecal(olc::vf2d{ 0.0f,0.0f }, olc::vf2d{ (float)ScreenWidth(), (float)ScreenHeight() }, olc::Pixel(50, 50, 50, 150));
			std::string message = "You Died!";
			DrawStringDecal(olc::vi2d{ (ScreenWidth() - (int)message.length() * 60) >> 1, (ScreenHeight() - 60) >> 1 }, message, olc::RED, olc::vf2d{ 7.0f, 7.0f });

			if (GetKey(olc::SPACE).bPressed) {
				ResetGame();
			}
		}

		return true;
	}
};

int main() {
	FlappyBird game;
	if (game.Construct(700, 1000, 1, 1))
		game.Start();
	return 0;
}