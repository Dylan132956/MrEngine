
#pragma once

#include "hlslcc_lib/ShaderCompilerCommon.h"

#include "hlslcc_lib/hlslcc.h"
#include "hlslcc_lib/LanguageSpec.h"

class FGlslLanguageSpec : public ILanguageSpec
{
protected:
	bool bIsES2;
	bool bIsWebGL;
	bool bIsES31;

public:
	FGlslLanguageSpec(bool bInIsES2, bool bInIsWebGL, bool bInIsES31) 
		: bIsES2(bInIsES2)
		, bIsWebGL(bInIsWebGL) 
		, bIsES31(bInIsES31)
	{}

	virtual bool SupportsDeterminantIntrinsic() const override
	{
		return !bIsES2;
	}

	virtual bool SupportsTransposeIntrinsic() const override
	{
		return !bIsES2;
	}

	virtual bool SupportsIntegerModulo() const override
	{
		return !bIsES2 || bIsWebGL;
	}

	virtual bool SupportsMatrixConversions() const override { return true; }

	//#todo-rco: Enable
	virtual bool AllowsSharingSamplers() const override { return false; }

	virtual void SetupLanguageIntrinsics(_mesa_glsl_parse_state* State, exec_list* ir) override;

	virtual bool AllowsImageLoadsForNonScalar() const { return !bIsES2; }

	virtual bool EmulateStructuredWithTypedBuffers() const override { return bIsES31; }
};

class ir_variable;

// Generates GLSL compliant code from IR tokens
#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif // __GNUC__
struct FGlslCodeBackend : public FCodeBackend
{
	FGlslCodeBackend(unsigned int InHlslCompileFlags, EHlslCompileTarget InTarget, bool bInIsWebGL) :
		FCodeBackend(InHlslCompileFlags, InTarget),
		bIsWebGL(bInIsWebGL)
	{
	}

	virtual char* GenerateCode(struct exec_list* ir, struct _mesa_glsl_parse_state* ParseState, EHlslShaderFrequency Frequency) override;

	// Return false if there were restrictions that made compilation fail
	virtual bool ApplyAndVerifyPlatformRestrictions(exec_list* Instructions, _mesa_glsl_parse_state* ParseState, EHlslShaderFrequency Frequency) override;

	/**
	* Generate a GLSL main() function that calls the entry point and handles
	* reading and writing all input and output semantics.
	* @param Frequency - The shader frequency.
	* @param EntryPoint - The name of the shader entry point.
	* @param Instructions - IR code.
	* @param ParseState - Parse state.
	*/
	virtual bool GenerateMain(EHlslShaderFrequency Frequency, const char* EntryPoint, exec_list* Instructions, _mesa_glsl_parse_state* ParseState) override;

	void GenShaderPatchConstantFunctionInputs(_mesa_glsl_parse_state* ParseState, ir_variable* OutputPatchVar, exec_list &PostCallInstructions);

	void CallPatchConstantFunction(_mesa_glsl_parse_state* ParseState, ir_variable* OutputPatchVar, ir_function_signature* PatchConstantSig, exec_list& DeclInstructions, exec_list &PostCallInstructions);

	ir_function_signature* FindPatchConstantFunction(exec_list* Instructions, _mesa_glsl_parse_state* ParseState);


	// subclass functionality
	virtual bool AllowsGlobalUniforms()
	{
		return true;
	}

	virtual bool AllowsESLanguage()
	{
		return true;
	}

	virtual bool WantsPrecisionModifiers()
	{
		return Target == HCT_FeatureLevelES2 || Target == HCT_FeatureLevelES3_1 || Target == HCT_FeatureLevelES3_1Ext;
	}

	bool bIsWebGL;
};


#ifdef __GNUC__
#pragma GCC visibility pop
#endif // __GNUC__
