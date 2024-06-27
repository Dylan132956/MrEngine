#include "spirv_shader.h"
#include "shader_converter.h"
#include "glslang/MachineIndependent/gl_types.h"
#include "../Shader.h"

namespace moonriver
{
	void converter_spirv(int stage, const char* text[], const std::string fileName[],
		const char* fileNameList[], const char* entryPointName, int count, int option, std::vector<unsigned int>& spirv, Shader::Pass* pass)
	{
		glslang::TProgram program;
		CompileAndLinkShader((EShLanguage)stage, text, fileName, fileNameList, entryPointName, count, option, spirv, program);

		if (option & (1 << 8)) {
			int numUniformBlocks = program.getNumUniformBlocks();
			int uniformVariables = program.getNumUniformVariables();
			for (int i = 0; i < numUniformBlocks; ++i)
			{
				glslang::TObjectReflection ReflectionUniformBlock = program.getUniformBlock(i);
				Shader::Uniform u;
				u.name = ReflectionUniformBlock.name;
				u.binding = ReflectionUniformBlock.getBinding();
				u.size = ReflectionUniformBlock.size;
				int currSize = u.size;
				for (int j = uniformVariables - 1; j >= 0; --j)
				{
					glslang::TObjectReflection ReflectionVariables = program.getUniform(j);
					if (ReflectionUniformBlock.index == ReflectionVariables.index) {
						Shader::Member m;
						m.name = ReflectionVariables.name;
						m.offset = ReflectionVariables.offset;
						m.size = currSize - m.offset;
						currSize = m.offset;
						u.members.push_back(m);
					}
				}
				pass->uniforms.push_back(u);
			}

			Shader::SamplerGroup group;
			group.name = "PerMaterialFragment";
			group.binding = 4;
			for (int k = 0; k < uniformVariables; ++k)
			{
				glslang::TObjectReflection ReflectionVariables = program.getUniform(k);
				if (ReflectionVariables.glDefineType == GL_SAMPLER_2D)
				{
					glslang::TObjectReflection ReflectionVariables_next = program.getUniform(k + 1);
					Shader::Sampler s;
					s.name = "SPIRV_Cross_Combined" + ReflectionVariables.name + ReflectionVariables_next.name;
					s.binding = ReflectionVariables.getBinding();
					group.samplers.push_back(s);
				}
			}
			if (group.samplers.size() > 0) {
				pass->samplers.push_back(group);
			}
		}
	}
}