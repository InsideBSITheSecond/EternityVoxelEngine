#pragma once

#include "eve_model.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <memory>
#include <unordered_map>

namespace eve
{

	struct TransformComponent
	{
		glm::vec3 translation{};
		glm::vec3 scale{1.f, 1.f, 1.f};
		glm::vec3 rotation{};

		glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	struct PointLightComponent {
		float lightIntensity = 1.0f;
	};

	struct DirectionalLightComponent {
		float lightIntensity = 1.0f;
	};

	struct GravityComponent {
		JPH::BodyID bodyID;
		glm::vec3 direction;
		float force;
		bool grounded;
	};

	struct CollisionComponent {
		
	};

	struct EntityComponent {
		
	};

	struct AiComponent {
		
	};

	struct InventoryComponent {
		
	};

	class EveGameObject
	{
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, EveGameObject>;

		static EveGameObject createGameObject()
		{
			static id_t currentId = 0;
			return EveGameObject{currentId++};
		}

		static EveGameObject makePointLight(float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));
		static EveGameObject makeDirectionalLight(float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));
		static EveGameObject makeGravityObject(glm::vec3 direction, float force);

		~EveGameObject();

		EveGameObject(const EveGameObject &) = delete;
		EveGameObject &operator=(const EveGameObject &) = delete;
		EveGameObject(EveGameObject &&) = default;
		EveGameObject &operator=(EveGameObject &&) = default;

		id_t getId() { return id; }

		TransformComponent transform{};
		glm::vec3 color{};

		// Optional pointer components
		std::shared_ptr<EveModel> model{};
		std::shared_ptr<CollisionComponent> collisionComponent = nullptr;
		std::shared_ptr<EntityComponent> entityComponent = nullptr;
		std::shared_ptr<AiComponent> aiComponent = nullptr;
		std::shared_ptr<GravityComponent> gravityComponent = nullptr;
		std::unique_ptr<PointLightComponent> pointLightComponent = nullptr;
		std::unique_ptr<DirectionalLightComponent> directionalLightComponent = nullptr;
	private:
		EveGameObject(id_t objId) : id{objId} {}

		id_t id;
	};
}