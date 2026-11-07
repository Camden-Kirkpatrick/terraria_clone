#include "slime.hpp"
#include "../assetManager.hpp"
#include "../helpers.hpp"
#include "../randomStuff.hpp"

void Slime::render(AssetManager& assetManager)
{
	auto aabb = getRectangleForEntity(physics.transform, 1, 1);

	DrawTexturePro(
		assetManager.slime,
		getTextureAtlas(0, 0, 32, 32),
		aabb,
		{ 0, 0 },
		0.0f,
		WHITE
	);
}

void Slime::update(float deltaTime)
{

}