#pragma once

#include <cstdint>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <map>
#include <string>
#include <memory>

#include "UI/Utils.h"

namespace esx {

	class Shader {
	public:
		Shader(DataBuffer vertexSource, DataBuffer fragmentSource);
		~Shader();

		void start();
		void stop();

		void uploadUniform(const char* uniformName, const glm::uvec2& vec);
		void uploadUniform(const char* uniformName, const glm::ivec2& vec);
		void uploadUniform(const char* uniformName, const glm::mat4& mat);
		void uploadUniform(const char* uniformName, float value);
		void uploadUniform(const char* uniformName, int value);


		static std::shared_ptr<Shader> LoadFromFile(const char* vertexPath, const char* fragmentPath);

	private:
		int32_t getLocation(const char* uniformName);
	private:
		uint32_t mRendererID;
		std::map<std::string, int32_t> mLocationsMap;
	};

}