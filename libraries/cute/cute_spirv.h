/*
	------------------------------------------------------------------------------
		Licensing information can be found at the end of the file.
	------------------------------------------------------------------------------

	cute_spirv.h - v1.00

	To create implementation (the function definitions)
		#define CUTE_SPIRV_IMPLEMENTATION
	in *one* C/CPP file (translation unit) that includes this file

	HARD DEPENDENCY

		This header requires cute/ckit.h (strings, interning, hashtables, dynamic
		arrays) to be included *before* it, and CKIT_IMPLEMENTATION must be provided
		by some TU in the program:

			#define CKIT_IMPLEMENTATION // (in exactly one TU)
			#include "cute/ckit.h"
			#define CUTE_SPIRV_IMPLEMENTATION
			#include "cute/cute_spirv.h"

	SUMMARY

		cute_spirv.h is a single-file header that compiles a well-defined subset of
		GLSL 450 ("CF-GLSL", see Cute Framework's docs/topics/glsl_support.md)
		directly to SPIR-V 1.3 for Vulkan 1.1+, with zero external dependencies.

		It performs zero optimization by design. Every terminal consumer of a
		shader (Vulkan drivers, Metal's compiler, D3D compilers, GL drivers,
		SPIRV-Cross) runs its own full optimizing compiler, so emission here is
		deliberately naive: locals are OpVariables, expressions are load/store
		straight-line code, no SSA construction, no optimizer. This keeps the
		compiler small, fast, and easy to hack on.

		Usage:

			CSPV_Result result = cspv_compile(source, CSPV_STAGE_FRAGMENT);
			if (result.success) { use result.spirv / result.word_count / result.reflection }
			else                { report result.error_message                              }
			cspv_free(&result);

		Use cspv_compile_ex to provide #include resolution, preprocessor defines,
		and to request the preprocessed source.

	LIMITATIONS

		Supports vertex, fragment, and compute stages and the GLSL feature set
		used by Cute Framework's shaders (full expression grammar, control flow,
		functions with overloads, structs, arrays, matrices, uniform blocks,
		SSBOs with runtime array tails, samplers, storage images, atomics,
		barriers, shared memory, and a full preprocessor with #include).

		Not supported: doubles, non-square matrices, geometry/tessellation
		stages, sampler3D/samplerCube/shadow samplers, multi-dimensional arrays,
		out/inout function parameters, # and ## preprocessor operators. The
		subset is documented precisely in CF's docs and grows on request.

	Revision history:
		1.00 (07/21/2026) initial release: preprocessor, full CF-GLSL subset,
		                  reflection, 700+ check test suite
*/
#ifndef CUTE_SPIRV_H
#define CUTE_SPIRV_H

#ifndef CKIT_H
#error "cute_spirv.h requires cute/ckit.h -- include it first"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CSPV_Stage
{
	CSPV_STAGE_VERTEX,
	CSPV_STAGE_FRAGMENT,
	CSPV_STAGE_COMPUTE,
} CSPV_Stage;

// Data types for reflection. Values intentionally match CF_ShaderInfoDataType in
// cute_shader_bytecode.h so the CF glue can cast directly.
typedef enum CSPV_DataType
{
	CSPV_TYPE_UNKNOWN = 0,
	CSPV_TYPE_SINT    = 1,
	CSPV_TYPE_UINT    = 2,
	CSPV_TYPE_FLOAT   = 3,
	CSPV_TYPE_SINT2   = 4,
	CSPV_TYPE_UINT2   = 5,
	CSPV_TYPE_FLOAT2  = 6,
	CSPV_TYPE_SINT3   = 7,
	CSPV_TYPE_UINT3   = 8,
	CSPV_TYPE_FLOAT3  = 9,
	CSPV_TYPE_SINT4   = 10,
	CSPV_TYPE_UINT4   = 11,
	CSPV_TYPE_FLOAT4  = 12,
	CSPV_TYPE_MAT4    = 13,
} CSPV_DataType;

typedef struct CSPV_ReflectionResource
{
	const char* name; // Interned; outlives the result.
	int set;
	int binding;
	bool readonly;
} CSPV_ReflectionResource;

typedef struct CSPV_ReflectionMember
{
	const char* name;
	CSPV_DataType type;
	int offset;
	int array_length; // 1 for non-arrays.
} CSPV_ReflectionMember;

typedef struct CSPV_ReflectionBlock
{
	const char* name;
	int set;
	int binding;
	int size;         // std140 size in bytes.
	int num_members;
	int first_member; // Index into CSPV_Reflection.members.
} CSPV_ReflectionBlock;

typedef struct CSPV_ReflectionInput
{
	const char* name;
	int location;
	CSPV_DataType type;
} CSPV_ReflectionInput;

typedef struct CSPV_Reflection
{
	CK_DYNA CSPV_ReflectionResource* samplers;        // Combined image samplers.
	CK_DYNA CSPV_ReflectionResource* storage_images;  // image2D uniforms.
	CK_DYNA CSPV_ReflectionResource* storage_buffers; // SSBOs.
	CK_DYNA CSPV_ReflectionBlock* uniform_blocks;
	CK_DYNA CSPV_ReflectionMember* uniform_members;   // All blocks, tightly packed.
	CK_DYNA CSPV_ReflectionInput* inputs;             // Vertex stage attribute inputs.
} CSPV_Reflection;

typedef struct CSPV_Result
{
	bool success;

	// On failure - a ckit string with file:line info.
	CK_SDYNA char* error_message;

	// On success - the SPIR-V module as a ckit dynamic array (word_count == asize).
	CK_DYNA uint32_t* spirv;
	size_t word_count;

	// On success - reflection data (ckit arrays; names are interned strings).
	CSPV_Reflection reflection;

	// On success, when CSPV_Options.return_preprocessed was set - a ckit string.
	CK_SDYNA char* preprocessed;
} CSPV_Result;

typedef struct CSPV_Define
{
	const char* name;
	const char* value; // May be NULL (defined with no value, e.g. for #ifdef).
} CSPV_Define;

typedef struct CSPV_Options
{
	int num_defines;
	CSPV_Define* defines;

	// Resolve an #include to file content, or return NULL if not found. The returned
	// string must stay alive for the duration of cspv_compile_ex. Each unique path is
	// included at most once (automatic include guards, matching CF's shader system).
	const char* (*include_resolve)(const char* path, void* user);
	void* user;

	// When set, CSPV_Result.preprocessed carries the preprocessed source.
	bool return_preprocessed;

	// When set, stop after preprocessing: CSPV_Result.preprocessed is filled and no
	// parsing or code generation runs (spirv/reflection stay empty). Useful for
	// resource scanning over comment-stripped, macro-expanded source.
	bool preprocess_only;

	// Optional: map an include path to a different name in error messages (e.g.
	// show the user's shader path instead of an internal stub name). Return NULL
	// to keep the path as-is.
	const char* (*display_name)(const char* path, void* user);
} CSPV_Options;

CSPV_Result cspv_compile(const char* source, CSPV_Stage stage);
CSPV_Result cspv_compile_ex(const char* source, CSPV_Stage stage, const CSPV_Options* opts);
void cspv_free(CSPV_Result* result);

#ifdef __cplusplus
}
#endif

#endif // CUTE_SPIRV_H

//--------------------------------------------------------------------------------------------------

#ifdef CUTE_SPIRV_IMPLEMENTATION
#ifndef CUTE_SPIRV_IMPLEMENTATION_ONCE
#define CUTE_SPIRV_IMPLEMENTATION_ONCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>

//--------------------------------------------------------------------------------------------------
// SPIR-V constants. Hand-written from the SPIR-V 1.3 spec so no external headers are needed.

#define CSPV_MAGIC   0x07230203u
#define CSPV_VERSION 0x00010300u // 1.3

enum
{
	CSpvOpName                 = 5,
	CSpvOpMemberName           = 6,
	CSpvOpExtInstImport        = 11,
	CSpvOpExtInst              = 12,
	CSpvOpMemoryModel          = 14,
	CSpvOpEntryPoint           = 15,
	CSpvOpExecutionMode        = 16,
	CSpvOpCapability           = 17,
	CSpvOpTypeVoid             = 19,
	CSpvOpTypeBool             = 20,
	CSpvOpTypeInt              = 21,
	CSpvOpTypeFloat            = 22,
	CSpvOpTypeVector           = 23,
	CSpvOpTypeMatrix           = 24,
	CSpvOpTypeImage            = 25,
	CSpvOpTypeSampledImage     = 27,
	CSpvOpTypeArray            = 28,
	CSpvOpTypeRuntimeArray     = 29,
	CSpvOpTypeStruct           = 30,
	CSpvOpTypePointer          = 32,
	CSpvOpTypeFunction         = 33,
	CSpvOpDecorate             = 71,
	CSpvOpMemberDecorate       = 72,
	CSpvOpConstantTrue         = 41,
	CSpvOpConstantFalse        = 42,
	CSpvOpConstant             = 43,
	CSpvOpConstantComposite    = 44,
	CSpvOpFunction             = 54,
	CSpvOpFunctionParameter    = 55,
	CSpvOpFunctionEnd          = 56,
	CSpvOpFunctionCall         = 57,
	CSpvOpVariable             = 59,
	CSpvOpLoad                 = 61,
	CSpvOpStore                = 62,
	CSpvOpAccessChain          = 65,
	CSpvOpArrayLength          = 68,
	CSpvOpVectorShuffle        = 79,
	CSpvOpCompositeConstruct   = 80,
	CSpvOpCompositeExtract     = 81,
	CSpvOpCompositeInsert      = 82,
	CSpvOpTranspose            = 84,
	CSpvOpImageSampleImplicitLod = 87,
	CSpvOpImageSampleExplicitLod = 88,
	CSpvOpImageFetch           = 95,
	CSpvOpImageRead            = 98,
	CSpvOpImageWrite           = 99,
	CSpvOpImage                = 100,
	CSpvOpImageQuerySizeLod    = 103,
	CSpvOpImageQuerySize       = 104,
	CSpvOpConvertFToU          = 109,
	CSpvOpConvertFToS          = 110,
	CSpvOpConvertSToF          = 111,
	CSpvOpConvertUToF          = 112,
	CSpvOpBitcast              = 124,
	CSpvOpSNegate              = 126,
	CSpvOpFNegate              = 127,
	CSpvOpIAdd                 = 128,
	CSpvOpFAdd                 = 129,
	CSpvOpISub                 = 130,
	CSpvOpFSub                 = 131,
	CSpvOpIMul                 = 132,
	CSpvOpFMul                 = 133,
	CSpvOpUDiv                 = 134,
	CSpvOpSDiv                 = 135,
	CSpvOpFDiv                 = 136,
	CSpvOpUMod                 = 137,
	CSpvOpSMod                 = 139,
	CSpvOpFMod                 = 141,
	CSpvOpVectorTimesScalar    = 142,
	CSpvOpMatrixTimesScalar    = 143,
	CSpvOpVectorTimesMatrix    = 144,
	CSpvOpMatrixTimesVector    = 145,
	CSpvOpMatrixTimesMatrix    = 146,
	CSpvOpDot                  = 148,
	CSpvOpAny                  = 154,
	CSpvOpAll                  = 155,
	CSpvOpLogicalEqual         = 164,
	CSpvOpLogicalNotEqual      = 165,
	CSpvOpLogicalOr            = 166,
	CSpvOpLogicalAnd           = 167,
	CSpvOpLogicalNot           = 168,
	CSpvOpSelect               = 169,
	CSpvOpIEqual               = 170,
	CSpvOpINotEqual            = 171,
	CSpvOpUGreaterThan         = 172,
	CSpvOpSGreaterThan         = 173,
	CSpvOpUGreaterThanEqual    = 174,
	CSpvOpSGreaterThanEqual    = 175,
	CSpvOpULessThan            = 176,
	CSpvOpSLessThan            = 177,
	CSpvOpULessThanEqual       = 178,
	CSpvOpSLessThanEqual       = 179,
	CSpvOpFOrdEqual            = 180,
	CSpvOpFUnordNotEqual       = 183,
	CSpvOpFOrdLessThan         = 184,
	CSpvOpFOrdGreaterThan      = 186,
	CSpvOpFOrdLessThanEqual    = 188,
	CSpvOpFOrdGreaterThanEqual = 190,
	CSpvOpShiftRightLogical    = 194,
	CSpvOpShiftRightArithmetic = 195,
	CSpvOpShiftLeftLogical     = 196,
	CSpvOpBitwiseOr            = 197,
	CSpvOpBitwiseXor           = 198,
	CSpvOpBitwiseAnd           = 199,
	CSpvOpNot                  = 200,
	CSpvOpDPdx                 = 207,
	CSpvOpDPdy                 = 208,
	CSpvOpFwidth               = 209,
	CSpvOpControlBarrier       = 224,
	CSpvOpMemoryBarrier        = 225,
	CSpvOpAtomicExchange       = 229,
	CSpvOpAtomicCompareExchange = 230,
	CSpvOpAtomicIAdd           = 234,
	CSpvOpAtomicSMin           = 236,
	CSpvOpAtomicUMin           = 237,
	CSpvOpAtomicSMax           = 238,
	CSpvOpAtomicUMax           = 239,
	CSpvOpAtomicAnd            = 240,
	CSpvOpAtomicOr             = 241,
	CSpvOpAtomicXor            = 242,
	CSpvOpLoopMerge            = 246,
	CSpvOpSelectionMerge       = 247,
	CSpvOpLabel                = 248,
	CSpvOpBranch               = 249,
	CSpvOpBranchConditional    = 250,
	CSpvOpSwitch               = 251,
	CSpvOpKill                 = 252,
	CSpvOpReturn               = 253,
	CSpvOpReturnValue          = 254,
	CSpvOpUnreachable          = 255,
};

// GLSL.std.450 extended instruction numbers.
enum
{
	GLSLstd450Round         = 1,
	GLSLstd450Trunc         = 3,
	GLSLstd450FAbs          = 4,
	GLSLstd450SAbs          = 5,
	GLSLstd450FSign         = 6,
	GLSLstd450SSign         = 7,
	GLSLstd450Floor         = 8,
	GLSLstd450Ceil          = 9,
	GLSLstd450Fract         = 10,
	GLSLstd450Sin           = 13,
	GLSLstd450Cos           = 14,
	GLSLstd450Tan           = 15,
	GLSLstd450Asin          = 16,
	GLSLstd450Acos          = 17,
	GLSLstd450Atan          = 18,
	GLSLstd450Atan2         = 25,
	GLSLstd450Pow           = 26,
	GLSLstd450Exp           = 27,
	GLSLstd450Log           = 28,
	GLSLstd450Exp2          = 29,
	GLSLstd450Log2          = 30,
	GLSLstd450Sqrt          = 31,
	GLSLstd450InverseSqrt   = 32,
	GLSLstd450Determinant   = 33,
	GLSLstd450MatrixInverse = 34,
	GLSLstd450FMin          = 37,
	GLSLstd450UMin          = 38,
	GLSLstd450SMin          = 39,
	GLSLstd450FMax          = 40,
	GLSLstd450UMax          = 41,
	GLSLstd450SMax          = 42,
	GLSLstd450FClamp        = 43,
	GLSLstd450UClamp        = 44,
	GLSLstd450SClamp        = 45,
	GLSLstd450FMix          = 46,
	GLSLstd450Step          = 48,
	GLSLstd450SmoothStep    = 49,
	GLSLstd450Length        = 66,
	GLSLstd450Distance      = 67,
	GLSLstd450Cross         = 68,
	GLSLstd450Normalize     = 69,
	GLSLstd450Reflect       = 71,
	GLSLstd450Refract       = 72,
};

// Storage classes.
enum
{
	CSpvStorageUniformConstant = 0,
	CSpvStorageInput           = 1,
	CSpvStorageUniform         = 2,
	CSpvStorageOutput          = 3,
	CSpvStoragePrivate         = 6,
	CSpvStorageFunction        = 7,
	CSpvStorageWorkgroup       = 4,
	CSpvStorageStorageBuffer   = 12,
};

// Decorations.
enum
{
	CSpvDecorationBlock         = 2,
	CSpvDecorationColMajor      = 5,
	CSpvDecorationArrayStride   = 6,
	CSpvDecorationMatrixStride  = 7,
	CSpvDecorationFlat          = 14,
	CSpvDecorationNonWritable   = 24,
	CSpvDecorationNonReadable   = 25,
	CSpvDecorationLocation      = 30,
	CSpvDecorationBinding       = 33,
	CSpvDecorationDescriptorSet = 34,
	CSpvDecorationOffset        = 35,
	CSpvDecorationBuiltIn       = 11,
};

enum
{
	CSpvBuiltInPosition  = 0,
	CSpvBuiltInFragCoord = 15,
};

//--------------------------------------------------------------------------------------------------
// Arena allocator. All compiler memory comes from here and is freed in one shot.

typedef struct cspv_arena_block
{
	struct cspv_arena_block* next;
	size_t used;
	size_t capacity;
	// Data follows.
} cspv_arena_block;

typedef struct cspv_arena
{
	cspv_arena_block* head;
} cspv_arena;

static void* cspv_arena_alloc(cspv_arena* arena, size_t size)
{
	size = (size + 15) & ~(size_t)15;
	cspv_arena_block* b = arena->head;
	if (!b || b->used + size > b->capacity) {
		size_t cap = 64 * 1024;
		if (cap < size) cap = size;
		b = (cspv_arena_block*)malloc(sizeof(cspv_arena_block) + cap);
		b->next = arena->head;
		b->used = 0;
		b->capacity = cap;
		arena->head = b;
	}
	void* p = (char*)(b + 1) + b->used;
	b->used += size;
	return p;
}

static void cspv_arena_free(cspv_arena* arena)
{
	cspv_arena_block* b = arena->head;
	while (b) {
		cspv_arena_block* next = b->next;
		free(b);
		b = next;
	}
	arena->head = NULL;
}

static char* cspv_arena_strndup(cspv_arena* arena, const char* s, size_t n)
{
	char* p = (char*)cspv_arena_alloc(arena, n + 1);
	memcpy(p, s, n);
	p[n] = 0;
	return p;
}

//--------------------------------------------------------------------------------------------------
// Lexer. Identifiers and keywords are interned via ckit's sintern so all later
// comparisons are pointer compares.

typedef enum cspv_tok_kind
{
	CSPV_TOK_EOF,
	CSPV_TOK_IDENT,      // Interned identifier (includes type names and keywords; parser disambiguates by pointer).
	CSPV_TOK_INT_LIT,
	CSPV_TOK_UINT_LIT,
	CSPV_TOK_FLOAT_LIT,
	CSPV_TOK_PUNCT,      // Single or multi-char punctuator, encoded in `punct`.
} cspv_tok_kind;

// Multi-char punctuators get compact codes; single-char ones are their ASCII value.
enum
{
	CSPV_P_SHL = 256,   // <<
	CSPV_P_SHR,         // >>
	CSPV_P_LE,          // <=
	CSPV_P_GE,          // >=
	CSPV_P_EQ,          // ==
	CSPV_P_NE,          // !=
	CSPV_P_AND,         // &&
	CSPV_P_OR,          // ||
	CSPV_P_ADD_ASSIGN,  // +=
	CSPV_P_SUB_ASSIGN,  // -=
	CSPV_P_MUL_ASSIGN,  // *=
	CSPV_P_DIV_ASSIGN,  // /=
	CSPV_P_MOD_ASSIGN,  // %=
	CSPV_P_AND_ASSIGN,  // &=
	CSPV_P_OR_ASSIGN,   // |=
	CSPV_P_XOR_ASSIGN,  // ^=
	CSPV_P_SHL_ASSIGN,  // <<=
	CSPV_P_SHR_ASSIGN,  // >>=
	CSPV_P_INC,         // ++
	CSPV_P_DEC,         // --
};

typedef struct cspv_token
{
	cspv_tok_kind kind;
	int line;
	const char* ident;   // CSPV_TOK_IDENT: interned string.
	double float_val;    // CSPV_TOK_FLOAT_LIT
	uint64_t int_val;    // CSPV_TOK_INT_LIT / CSPV_TOK_UINT_LIT
	int punct;           // CSPV_TOK_PUNCT
} cspv_token;

//--------------------------------------------------------------------------------------------------
// Types. Scalar/vector/matrix types are canonical singletons in the context, so
// type equality is pointer equality.

typedef enum cspv_type_kind
{
	CSPV_T_VOID,
	CSPV_T_BOOL,
	CSPV_T_INT,
	CSPV_T_UINT,
	CSPV_T_FLOAT,
	CSPV_T_VEC,       // elem = scalar type, cols = 2..4
	CSPV_T_MAT,       // cols x rows, float only
	CSPV_T_SAMPLER2D,
	CSPV_T_ARRAY,     // elem = element type, cols = length (canonicalized per (elem, len); -1 = runtime).
	CSPV_T_STRUCT,    // Nominal; one cspv_type per declaration.
	CSPV_T_IMAGE2D,   // Storage image; cols = SPIR-V image format (-1 = formatless placeholder).
} cspv_type_kind;

typedef struct cspv_type
{
	cspv_type_kind kind;
	struct cspv_type* elem; // CSPV_T_VEC/ARRAY: element type.
	int cols;               // CSPV_T_VEC: components. CSPV_T_MAT: columns. CSPV_T_ARRAY: length.
	int rows;               // CSPV_T_MAT: rows.
	// CSPV_T_STRUCT:
	const char* name;
	CK_DYNA const char** field_names;
	CK_DYNA struct cspv_type** field_types;
} cspv_type;

//--------------------------------------------------------------------------------------------------
// AST.

typedef struct cspv_expr cspv_expr;
typedef struct cspv_stmt cspv_stmt;

typedef enum cspv_expr_kind
{
	CSPV_E_FLOAT_LIT,
	CSPV_E_INT_LIT,
	CSPV_E_UINT_LIT,
	CSPV_E_BOOL_LIT,
	CSPV_E_REF,
	CSPV_E_BINARY,    // Also assignments and logical and/or.
	CSPV_E_UNARY,
	CSPV_E_COND,      // Ternary.
	CSPV_E_CALL,      // Function call, intrinsic, or constructor.
	CSPV_E_MEMBER,    // Swizzle or struct field.
	CSPV_E_INDEX,     // Vector, matrix, or array subscript.
	CSPV_E_LENGTH,    // arr.length()
} cspv_expr_kind;

struct cspv_expr
{
	cspv_expr_kind kind;
	int line;
	union
	{
		double fval;
		uint64_t ival;
		bool bval;
		const char* name;                                        // CSPV_E_REF
		struct { int op; cspv_expr* l; cspv_expr* r; } bin;      // CSPV_E_BINARY
		struct { int op; cspv_expr* e; bool postfix; } un;       // CSPV_E_UNARY
		struct { cspv_expr* c; cspv_expr* a; cspv_expr* b; } cond;
		// array_size: -1 = plain call; 0 = unsized array ctor T[](...); N = T[N](...).
		struct { const char* name; CK_DYNA cspv_expr** args; int array_size; } call;
		struct { cspv_expr* base; const char* member; } member;
		struct { cspv_expr* base; cspv_expr* index; } index;
	} u;
};

typedef enum cspv_stmt_kind
{
	CSPV_S_DECL,
	CSPV_S_EXPR,
	CSPV_S_IF,
	CSPV_S_FOR,
	CSPV_S_WHILE,
	CSPV_S_DO,
	CSPV_S_SWITCH,
	CSPV_S_RETURN,
	CSPV_S_DISCARD,
	CSPV_S_BREAK,
	CSPV_S_CONTINUE,
	CSPV_S_BLOCK,
} cspv_stmt_kind;

struct cspv_stmt
{
	cspv_stmt_kind kind;
	int line;
	union
	{
		struct { cspv_type* type; const char* name; cspv_expr* init; cspv_stmt* next_decl; } decl; // next_decl: `int i, j;` chains.
		cspv_expr* expr;                                              // CSPV_S_EXPR
		struct { cspv_expr* cond; cspv_stmt* then_s; cspv_stmt* else_s; } if_s;
		struct { cspv_stmt* init; cspv_expr* cond; cspv_expr* iter; cspv_stmt* body; } for_s;
		struct { cspv_expr* cond; cspv_stmt* body; } while_s; // Also CSPV_S_DO.
		struct { cspv_expr* sel; struct cspv_switch_group* groups; } switch_s;
		cspv_expr* ret;                                               // CSPV_S_RETURN (may be NULL)
		CK_DYNA cspv_stmt** block;                                    // CSPV_S_BLOCK
	} u;
};

// One `case ...:` (or `default:`) group in a switch, with its statements.
typedef struct cspv_switch_group
{
	CK_DYNA int64_t* labels; // Case literal values.
	bool is_default;
	CK_DYNA cspv_stmt** stmts;
} cspv_switch_group;

//--------------------------------------------------------------------------------------------------
// Symbols.

typedef enum cspv_sym_kind
{
	CSPV_SYM_VAR,          // Local or global variable backed by a pointer id.
	CSPV_SYM_BLOCK_MEMBER, // Member of an anonymous uniform block.
	CSPV_SYM_FUNC,
	CSPV_SYM_GL_POSITION,  // Accessed via gl_PerVertex member 0.
} cspv_sym_kind;

typedef struct cspv_symbol
{
	cspv_sym_kind kind;
	const char* name;
	cspv_type* type;
	uint32_t id;            // Pointer id (vars), function id (funcs), block var id (members).
	int storage;            // Storage class for vars/members.
	int member_index;       // CSPV_SYM_BLOCK_MEMBER
	uint32_t laid_tid;      // CSPV_SYM_BLOCK_MEMBER of struct/struct-array type: laid-out tid.
	// Constant scalars (const globals): folded value for constant expressions.
	bool has_const_value;
	uint32_t const_bits;
	// Functions:
	CK_DYNA cspv_type** params;
	CK_DYNA int* param_quals; // 0 = in, 1 = out, 2 = inout.
	struct cspv_symbol* next_overload;
} cspv_symbol;

typedef struct cspv_scope
{
	CK_MAP(cspv_symbol*) syms;
} cspv_scope;

//--------------------------------------------------------------------------------------------------
// Compiler context.

typedef struct cspv_ctx
{
	cspv_arena arena;
	jmp_buf jmp;
	CK_SDYNA char* error; // Set on failure; ownership passes to CSPV_Result.
	CSPV_Stage stage;

	// Lexer.
	const char* p;
	int line;
	cspv_token tok;
	cspv_token ahead[4];
	int ahead_count;

	// Source files seen by the preprocessor. files[0] is the primary source
	// ("shader"); the rest are #include paths. file_index tracks the lexer's
	// current file via `#l` line markers in the preprocessed text.
	CK_DYNA const char** files;
	int file_index;
	CK_SDYNA char* pp_text; // Preprocessed source (owned; freed in cleanup).

	// Canonical types.
	cspv_type* t_void;
	cspv_type* t_bool;
	cspv_type* t_int;
	cspv_type* t_uint;
	cspv_type* t_float;
	cspv_type* t_vec[3];   // vec2..vec4
	cspv_type* t_ivec[3];
	cspv_type* t_uvec[3];
	cspv_type* t_bvec[3];
	cspv_type* t_mat[3];   // mat2..mat4
	cspv_type* t_sampler2d;
	cspv_type* t_usampler2d; // elem = uint (texelFetch yields uvec4).
	CK_MAP(cspv_type*) type_names;  // interned name -> type
	CK_MAP(cspv_type*) array_types; // (elem ptr, len) -> canonical array type
	CK_MAP(cspv_type*) image_types; // format -> canonical image2D type
	int local_size[3];              // Compute workgroup size (0 = undeclared).

	// SPIR-V module state.
	uint32_t next_id;
	uint32_t glsl_ext_id;
	CK_DYNA uint32_t* names;      // OpName/OpMemberName
	CK_DYNA uint32_t* decos;      // Annotations
	CK_DYNA uint32_t* globals;    // Types, constants, global variables
	CK_DYNA uint32_t* funcs;      // Completed function bodies
	CK_DYNA uint32_t* body;       // Current function: instructions after the entry label
	CK_DYNA uint32_t* local_vars; // Current function: OpVariables for the entry block
	CK_DYNA uint32_t* interface_ids; // Input/Output vars for OpEntryPoint
	CK_MAP(uint32_t) type_ids;     // cspv_type* -> spirv id
	CK_MAP(uint32_t) ptr_type_ids; // (type_id << 4) | storage -> spirv id
	CK_MAP(uint32_t) const_ids;    // ((uint64_t)type_id << 32) | bits -> spirv id
	CK_MAP(uint32_t) fn_type_ids;  // signature hash -> spirv id
	// Buffer-block interior structs need distinct spirv types carrying std430 Offset
	// decorations (types with offsets may not be used in Function storage, so the
	// plain struct type cannot be reused). Keyed by cspv_type* -> laid-out tid.
	CK_MAP(uint32_t) laid_struct_tids;
	// Runtime arrays of laid-out structs, keyed the same way.
	CK_MAP(uint32_t) laid_array_tids;
	// laid-out array tid -> its element's laid-out struct tid.
	CK_MAP(uint32_t) laid_elem_tids;
	uint32_t sampler2d_image_tid;  // OpTypeImage id behind sampler2D (for texelFetch).
	uint32_t usampler2d_image_tid; // Same for usampler2D.
	bool needs_image_query;        // Emit OpCapability ImageQuery (textureSize).
	bool needs_extended_formats;   // Emit OpCapability StorageImageExtendedFormats.

	// Codegen state.
	CK_DYNA cspv_scope* scopes;
	CK_MAP(cspv_symbol*) functions; // interned name -> overload chain head
	cspv_symbol* cur_func;
	bool func_has_return; // Current function contains at least one return statement.
	bool block_terminated;
	uint32_t break_target;
	uint32_t continue_target;
	uint32_t entry_func_id;
	uint32_t gl_pervertex_var;

	// Reflection, built up during global declaration codegen. Ownership transfers
	// to CSPV_Result on success.
	CSPV_Reflection reflection;
} cspv_ctx;

//--------------------------------------------------------------------------------------------------
// Errors. Compilation bails via longjmp on the first error.

// Token/AST lines carry their source file in the high digits so errors report the
// right file across #includes: encoded = file_index * 1000000 + line.
#define CSPV_LINE(ctx, raw_line) ((ctx)->file_index * 1000000 + (raw_line))

static void cspv_errorf(cspv_ctx* ctx, int line, const char* fmt, ...)
{
	int file_index = line / 1000000;
	line %= 1000000;
	const char* file = "shader";
	if (ctx->files && file_index < (int)asize(ctx->files)) file = ctx->files[file_index];

	CK_SDYNA char* msg = sfmake("%s:%d: error: ", file, line);
	va_list args;
	va_start(args, fmt);
	svfmt_append(msg, fmt, args);
	va_end(args);
	ctx->error = msg; // Handed to the caller via CSPV_Result (freed by cspv_free).
	longjmp(ctx->jmp, 1);
}

//--------------------------------------------------------------------------------------------------
// Lexer implementation.

static void cspv_lex_next(cspv_ctx* ctx, cspv_token* t)
{
	const char* p = ctx->p;
	for (;;) {
		while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
			if (*p == '\n') ctx->line++;
			p++;
		}
		if (p[0] == '/' && p[1] == '/') {
			while (*p && *p != '\n') p++;
			continue;
		}
		if (p[0] == '/' && p[1] == '*') {
			p += 2;
			while (*p && !(p[0] == '*' && p[1] == '/')) {
				if (*p == '\n') ctx->line++;
				p++;
			}
			if (*p) p += 2;
			continue;
		}
		// Line marker emitted by the preprocessor: `#l <line> <file_index>`. The text
		// after the marker's own newline starts at exactly <line>, so that newline is
		// consumed without counting.
		if (p[0] == '#' && p[1] == 'l' && (p[2] == ' ' || p[2] == '\t')) {
			p += 3;
			ctx->line = (int)strtol(p, (char**)&p, 10);
			ctx->file_index = (int)strtol(p, (char**)&p, 10);
			while (*p && *p != '\n') p++;
			if (*p == '\n') p++;
			continue;
		}
		break;
	}

	memset(t, 0, sizeof(*t));
	t->line = CSPV_LINE(ctx, ctx->line);

	if (*p == 0) {
		t->kind = CSPV_TOK_EOF;
		ctx->p = p;
		return;
	}

	// Identifier / keyword.
	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_') {
		const char* start = p;
		while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_') p++;
		t->kind = CSPV_TOK_IDENT;
		t->ident = sintern_range(start, p);
		ctx->p = p;
		return;
	}

	// Number.
	if ((*p >= '0' && *p <= '9') || (*p == '.' && p[1] >= '0' && p[1] <= '9')) {
		const char* start = p;
		bool is_float = false;
		if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
			p += 2;
			while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) p++;
		} else {
			while (*p >= '0' && *p <= '9') p++;
			if (*p == '.') {
				is_float = true;
				p++;
				while (*p >= '0' && *p <= '9') p++;
			}
			if (*p == 'e' || *p == 'E') {
				is_float = true;
				p++;
				if (*p == '+' || *p == '-') p++;
				while (*p >= '0' && *p <= '9') p++;
			}
		}
		if (is_float) {
			t->kind = CSPV_TOK_FLOAT_LIT;
			t->float_val = strtod(start, NULL);
			if (*p == 'f' || *p == 'F') p++;
		} else {
			if (*p == 'f' || *p == 'F') {
				t->kind = CSPV_TOK_FLOAT_LIT;
				t->float_val = strtod(start, NULL);
				p++;
			} else {
				uint64_t v = strtoull(start, NULL, 0);
				if (*p == 'u' || *p == 'U') {
					p++;
					t->kind = CSPV_TOK_UINT_LIT;
				} else {
					t->kind = CSPV_TOK_INT_LIT;
				}
				t->int_val = v;
			}
		}
		ctx->p = p;
		return;
	}

	// Punctuators.
	t->kind = CSPV_TOK_PUNCT;
	int c = *p++;
	int c2 = *p;
	switch (c) {
	case '<':
		if (c2 == '<') { p++; if (*p == '=') { p++; t->punct = CSPV_P_SHL_ASSIGN; } else t->punct = CSPV_P_SHL; }
		else if (c2 == '=') { p++; t->punct = CSPV_P_LE; }
		else t->punct = '<';
		break;
	case '>':
		if (c2 == '>') { p++; if (*p == '=') { p++; t->punct = CSPV_P_SHR_ASSIGN; } else t->punct = CSPV_P_SHR; }
		else if (c2 == '=') { p++; t->punct = CSPV_P_GE; }
		else t->punct = '>';
		break;
	case '=': if (c2 == '=') { p++; t->punct = CSPV_P_EQ; } else t->punct = '='; break;
	case '!': if (c2 == '=') { p++; t->punct = CSPV_P_NE; } else t->punct = '!'; break;
	case '&':
		if (c2 == '&') { p++; t->punct = CSPV_P_AND; }
		else if (c2 == '=') { p++; t->punct = CSPV_P_AND_ASSIGN; }
		else t->punct = '&';
		break;
	case '|':
		if (c2 == '|') { p++; t->punct = CSPV_P_OR; }
		else if (c2 == '=') { p++; t->punct = CSPV_P_OR_ASSIGN; }
		else t->punct = '|';
		break;
	case '+':
		if (c2 == '+') { p++; t->punct = CSPV_P_INC; }
		else if (c2 == '=') { p++; t->punct = CSPV_P_ADD_ASSIGN; }
		else t->punct = '+';
		break;
	case '-':
		if (c2 == '-') { p++; t->punct = CSPV_P_DEC; }
		else if (c2 == '=') { p++; t->punct = CSPV_P_SUB_ASSIGN; }
		else t->punct = '-';
		break;
	case '*': if (c2 == '=') { p++; t->punct = CSPV_P_MUL_ASSIGN; } else t->punct = '*'; break;
	case '/': if (c2 == '=') { p++; t->punct = CSPV_P_DIV_ASSIGN; } else t->punct = '/'; break;
	case '%': if (c2 == '=') { p++; t->punct = CSPV_P_MOD_ASSIGN; } else t->punct = '%'; break;
	case '^': if (c2 == '=') { p++; t->punct = CSPV_P_XOR_ASSIGN; } else t->punct = '^'; break;
	default:
		t->punct = c;
		break;
	}
	ctx->p = p;
}

static void cspv_advance(cspv_ctx* ctx)
{
	if (ctx->ahead_count) {
		ctx->tok = ctx->ahead[0];
		for (int i = 1; i < ctx->ahead_count; i++) ctx->ahead[i - 1] = ctx->ahead[i];
		ctx->ahead_count--;
	} else {
		cspv_lex_next(ctx, &ctx->tok);
	}
}

// Peek n tokens ahead (n = 1..4). Peek(0) is the current token.
static cspv_token* cspv_peek(cspv_ctx* ctx, int n)
{
	if (n == 0) return &ctx->tok;
	while (ctx->ahead_count < n) {
		cspv_lex_next(ctx, &ctx->ahead[ctx->ahead_count]);
		ctx->ahead_count++;
	}
	return &ctx->ahead[n - 1];
}

static bool cspv_is_punct(cspv_ctx* ctx, int punct)
{
	return ctx->tok.kind == CSPV_TOK_PUNCT && ctx->tok.punct == punct;
}

static bool cspv_accept_punct(cspv_ctx* ctx, int punct)
{
	if (cspv_is_punct(ctx, punct)) {
		cspv_advance(ctx);
		return true;
	}
	return false;
}

static void cspv_expect_punct(cspv_ctx* ctx, int punct)
{
	if (!cspv_is_punct(ctx, punct)) {
		if (punct < 256) cspv_errorf(ctx, ctx->tok.line, "expected '%c'", punct);
		else cspv_errorf(ctx, ctx->tok.line, "unexpected token");
	}
	cspv_advance(ctx);
}

static bool cspv_is_ident(cspv_ctx* ctx, const char* interned)
{
	return ctx->tok.kind == CSPV_TOK_IDENT && ctx->tok.ident == interned;
}

static bool cspv_accept_ident(cspv_ctx* ctx, const char* interned)
{
	if (cspv_is_ident(ctx, interned)) {
		cspv_advance(ctx);
		return true;
	}
	return false;
}

static const char* cspv_expect_any_ident(cspv_ctx* ctx)
{
	if (ctx->tok.kind != CSPV_TOK_IDENT) cspv_errorf(ctx, ctx->tok.line, "expected identifier");
	const char* name = ctx->tok.ident;
	cspv_advance(ctx);
	return name;
}

//--------------------------------------------------------------------------------------------------
// Preprocessor. Text-to-text, run before lexing. Supports #include (with automatic
// include guards, matching CF's shader system), object- and function-like #define,
// #undef, #ifdef/#ifndef/#if/#elif/#else/#endif with a constant-expression
// evaluator, and defined(). #version/#extension/#pragma are accepted and ignored.
// Stringize (#) and paste (##) are not supported.
//
// Output text contains `#l <line> <file_index>` markers which the lexer consumes to
// keep error locations accurate across includes and line continuations.

typedef struct cspv_macro
{
	const char* name; // Interned.
	bool is_function;
	CK_DYNA const char** params; // Interned parameter names.
	const char* body;            // Arena copy of the replacement text.
} cspv_macro;

typedef struct cspv_pp_cond
{
	bool parent_active;
	bool active;     // This branch currently emitting?
	bool taken;      // Has any branch of this #if chain been taken?
	bool seen_else;
} cspv_pp_cond;

// Interned directive/operator names, initialized once per process.
typedef struct cspv_pp_keywords
{
	const char* kw_include;
	const char* kw_define;
	const char* kw_undef;
	const char* kw_ifdef;
	const char* kw_ifndef;
	const char* kw_if;
	const char* kw_elif;
	const char* kw_else;
	const char* kw_endif;
	const char* kw_version;
	const char* kw_extension;
	const char* kw_pragma;
	const char* kw_line;
	const char* kw_defined;
} cspv_pp_keywords;

static cspv_pp_keywords g_cspv_pp_kw;

static void cspv_pp_init_keywords(void)
{
	if (g_cspv_pp_kw.kw_include) return;
	g_cspv_pp_kw.kw_include = sintern("include");
	g_cspv_pp_kw.kw_define = sintern("define");
	g_cspv_pp_kw.kw_undef = sintern("undef");
	g_cspv_pp_kw.kw_ifdef = sintern("ifdef");
	g_cspv_pp_kw.kw_ifndef = sintern("ifndef");
	g_cspv_pp_kw.kw_if = sintern("if");
	g_cspv_pp_kw.kw_elif = sintern("elif");
	g_cspv_pp_kw.kw_else = sintern("else");
	g_cspv_pp_kw.kw_endif = sintern("endif");
	g_cspv_pp_kw.kw_version = sintern("version");
	g_cspv_pp_kw.kw_extension = sintern("extension");
	g_cspv_pp_kw.kw_pragma = sintern("pragma");
	g_cspv_pp_kw.kw_line = sintern("line");
	g_cspv_pp_kw.kw_defined = sintern("defined");
}

typedef struct cspv_pp
{
	cspv_ctx* ctx;
	CK_MAP(cspv_macro*) macros;
	CK_MAP(int) included;              // Interned path -> 1.
	CK_DYNA cspv_pp_cond* conds;
	CK_DYNA const char** expanding;    // Macro names currently being expanded.
	const CSPV_Options* opts;
	CK_SDYNA char* out;
	int line;                          // Current line in the current file (for errors).
} cspv_pp;

static void cspv_pp_error(cspv_pp* pp, const char* fmt, ...)
{
	CK_SDYNA char* msg = NULL;
	va_list args;
	va_start(args, fmt);
	svfmt(msg, fmt, args);
	va_end(args);
	// cspv_errorf longjmps; msg intentionally leaks into the abandoned compile.
	cspv_errorf(pp->ctx, CSPV_LINE(pp->ctx, pp->line), "%s", msg);
}

static bool cspv_pp_active(cspv_pp* pp)
{
	for (int i = 0; i < (int)asize(pp->conds); i++) {
		if (!pp->conds[i].active) return false;
	}
	return true;
}

static bool cspv_is_ident_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static bool cspv_is_ident_start(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Strip comments from a whole file, preserving newlines (so line numbers hold).
static char* cspv_pp_strip_comments(cspv_ctx* ctx, const char* src)
{
	size_t len = strlen(src);
	char* out = (char*)cspv_arena_alloc(&ctx->arena, len + 1);
	size_t o = 0;
	for (size_t i = 0; i < len;) {
		if (src[i] == '/' && src[i + 1] == '/') {
			while (i < len && src[i] != '\n') i++;
		} else if (src[i] == '/' && src[i + 1] == '*') {
			i += 2;
			out[o++] = ' ';
			while (i < len && !(src[i] == '*' && src[i + 1] == '/')) {
				if (src[i] == '\n') out[o++] = '\n';
				i++;
			}
			if (i < len) i += 2;
		} else {
			out[o++] = src[i++];
		}
	}
	out[o] = 0;
	return out;
}

// Emit a line marker: the text following the marker line is at exactly `line` in
// `file_index`. (The lexer does not count the marker's own trailing newline.)
static void cspv_pp_marker(cspv_pp* pp, int line, int file_index)
{
	sfmt_append(pp->out, "\n#l %d %d\n", line, file_index);
}

// Expand macros in [start, end) into pp->out (or a returned string). When
// protect_defined is set, `defined(X)` / `defined X` pass through unexpanded.
static char* cspv_pp_expand_range(cspv_pp* pp, const char* start, const char* end, bool protect_defined);

// Expand into an existing dynamic string target. Appends.
static void cspv_pp_expand_into(cspv_pp* pp, CK_SDYNA char** target, const char* start, const char* end, bool protect_defined)
{
	CK_SDYNA char* expanded = cspv_pp_expand_range(pp, start, end, protect_defined);
	sappend(*target, expanded);
	sfree(expanded);
}

static bool cspv_pp_is_expanding(cspv_pp* pp, const char* name)
{
	for (int i = 0; i < (int)asize(pp->expanding); i++) {
		if (pp->expanding[i] == name) return true;
	}
	return false;
}

static char* cspv_pp_expand_range(cspv_pp* pp, const char* start, const char* end, bool protect_defined)
{
	char* out = smake("");
	const char* p = start;
	while (p < end) {
		char c = *p;
		if (cspv_is_ident_start(c)) {
			const char* id_start = p;
			while (p < end && cspv_is_ident_char(*p)) p++;
			const char* name = sintern_range(id_start, p);

			if (protect_defined && name == g_cspv_pp_kw.kw_defined) {
				// Copy `defined(X)` or `defined X` verbatim.
				sappend(out, "defined");
				const char* q = p;
				while (q < end && (*q == ' ' || *q == '\t')) q++;
				if (q < end && *q == '(') {
					while (p < end && *p != ')') spush(out, *p++);
					if (p < end) spush(out, *p++); // ')'
				} else {
					while (p < end && (*p == ' ' || *p == '\t')) spush(out, *p++);
					while (p < end && cspv_is_ident_char(*p)) spush(out, *p++);
				}
				continue;
			}

			cspv_macro* m = map_get(pp->macros, (uint64_t)(uintptr_t)name);
			if (!m || cspv_pp_is_expanding(pp, name)) {
				sappend_range(out, id_start, p);
				continue;
			}

			if (!m->is_function) {
				apush(pp->expanding, name);
				char* body = cspv_pp_expand_range(pp, m->body, m->body + strlen(m->body), protect_defined);
				apop(pp->expanding);
				sappend(out, body);
				sfree(body);
				continue;
			}

			// Function-like: require '(' or emit the name literally.
			const char* q = p;
			while (q < end && (*q == ' ' || *q == '\t' || *q == '\n')) q++;
			if (q >= end || *q != '(') {
				sappend_range(out, id_start, p);
				continue;
			}
			q++; // '('

			// Collect raw arguments at paren depth 0, splitting on top-level commas.
			CK_DYNA char** args = NULL;
			char* cur = smake("");
			int depth = 0;
			while (q < end) {
				char a = *q;
				if (a == '(') { depth++; spush(cur, a); q++; }
				else if (a == ')') {
					if (depth == 0) { q++; break; }
					depth--;
					spush(cur, a);
					q++;
				} else if (a == ',' && depth == 0) {
					apush(args, cur);
					cur = smake("");
					q++;
				} else {
					spush(cur, a);
					q++;
				}
			}
			apush(args, cur);
			p = q;

			if ((int)asize(args) != (int)asize(m->params) && !(asize(m->params) == 0 && asize(args) == 1 && slen(args[0]) == 0)) {
				cspv_pp_error(pp, "macro '%s' expects %d argument(s), got %d", name, (int)asize(m->params), (int)asize(args));
			}

			// Substitute parameters (raw), then rescan-expand the result.
			char* subst = smake("");
			const char* b = m->body;
			const char* bend = m->body + strlen(m->body);
			while (b < bend) {
				if (cspv_is_ident_start(*b)) {
					const char* is = b;
					while (b < bend && cspv_is_ident_char(*b)) b++;
					const char* bn = sintern_range(is, b);
					int param_index = -1;
					for (int i = 0; i < (int)asize(m->params); i++) {
						if (m->params[i] == bn) { param_index = i; break; }
					}
					if (param_index >= 0) sappend(subst, args[param_index]);
					else sappend_range(subst, is, b);
				} else {
					spush(subst, *b++);
				}
			}

			apush(pp->expanding, name);
			char* expanded = cspv_pp_expand_range(pp, subst, subst + slen(subst), protect_defined);
			apop(pp->expanding);
			sappend(out, expanded);
			sfree(expanded);
			sfree(subst);
			for (int i = 0; i < (int)asize(args); i++) sfree(args[i]);
			afree(args);
			continue;
		}

		// Numbers pass through as a unit so digits are never treated as macro names.
		if ((c >= '0' && c <= '9') || (c == '.' && p + 1 < end && p[1] >= '0' && p[1] <= '9')) {
			const char* ns = p;
			p++;
			while (p < end && (cspv_is_ident_char(*p) || *p == '.' ||
			       ((*p == '+' || *p == '-') && (p[-1] == 'e' || p[-1] == 'E')))) {
				p++;
			}
			sappend_range(out, ns, p);
			continue;
		}

		spush(out, c);
		p++;
	}
	return out;
}

//--------------------------------------------------------------------------------------------------
// #if expression evaluator (integers only, after macro expansion).

typedef struct cspv_pp_expr
{
	cspv_pp* pp;
	const char* p;
} cspv_pp_expr;

static long long cspv_ppx_ternary(cspv_pp_expr* x);

static void cspv_ppx_ws(cspv_pp_expr* x)
{
	while (*x->p == ' ' || *x->p == '\t') x->p++;
}

static long long cspv_ppx_primary(cspv_pp_expr* x)
{
	cspv_ppx_ws(x);
	char c = *x->p;
	if (c == '(') {
		x->p++;
		long long v = cspv_ppx_ternary(x);
		cspv_ppx_ws(x);
		if (*x->p != ')') cspv_pp_error(x->pp, "expected ')' in #if expression");
		x->p++;
		return v;
	}
	if (c == '!') { x->p++; return !cspv_ppx_primary(x); }
	if (c == '~') { x->p++; return ~cspv_ppx_primary(x); }
	if (c == '-') { x->p++; return -cspv_ppx_primary(x); }
	if (c == '+') { x->p++; return cspv_ppx_primary(x); }
	if (c >= '0' && c <= '9') {
		long long v = strtoll(x->p, (char**)&x->p, 0);
		// Swallow integer suffixes.
		while (*x->p == 'u' || *x->p == 'U' || *x->p == 'l' || *x->p == 'L') x->p++;
		return v;
	}
	if (cspv_is_ident_start(c)) {
		const char* s = x->p;
		while (cspv_is_ident_char(*x->p)) x->p++;
		if (sintern_range(s, x->p) == g_cspv_pp_kw.kw_defined) {
			cspv_ppx_ws(x);
			bool paren = *x->p == '(';
			if (paren) x->p++;
			cspv_ppx_ws(x);
			if (!cspv_is_ident_start(*x->p)) cspv_pp_error(x->pp, "expected identifier after 'defined'");
			const char* ns = x->p;
			while (cspv_is_ident_char(*x->p)) x->p++;
			const char* name = sintern_range(ns, x->p);
			if (paren) {
				cspv_ppx_ws(x);
				if (*x->p != ')') cspv_pp_error(x->pp, "expected ')' after defined(");
				x->p++;
			}
			return map_get(x->pp->macros, (uint64_t)(uintptr_t)name) != NULL;
		}
		return 0; // Unknown identifiers evaluate to 0, like C.
	}
	cspv_pp_error(x->pp, "bad #if expression");
	return 0;
}

static long long cspv_ppx_binary(cspv_pp_expr* x, int min_prec)
{
	long long lhs = cspv_ppx_primary(x);
	for (;;) {
		cspv_ppx_ws(x);
		const char* p = x->p;
		int prec = 0;
		int op = 0;
		if (p[0] == '*' ) { prec = 10; op = '*'; }
		else if (p[0] == '/') { prec = 10; op = '/'; }
		else if (p[0] == '%') { prec = 10; op = '%'; }
		else if (p[0] == '+') { prec = 9; op = '+'; }
		else if (p[0] == '-') { prec = 9; op = '-'; }
		else if (p[0] == '<' && p[1] == '<') { prec = 8; op = CSPV_P_SHL; }
		else if (p[0] == '>' && p[1] == '>') { prec = 8; op = CSPV_P_SHR; }
		else if (p[0] == '<' && p[1] == '=') { prec = 7; op = CSPV_P_LE; }
		else if (p[0] == '>' && p[1] == '=') { prec = 7; op = CSPV_P_GE; }
		else if (p[0] == '<') { prec = 7; op = '<'; }
		else if (p[0] == '>') { prec = 7; op = '>'; }
		else if (p[0] == '=' && p[1] == '=') { prec = 6; op = CSPV_P_EQ; }
		else if (p[0] == '!' && p[1] == '=') { prec = 6; op = CSPV_P_NE; }
		else if (p[0] == '&' && p[1] == '&') { prec = 2; op = CSPV_P_AND; }
		else if (p[0] == '|' && p[1] == '|') { prec = 1; op = CSPV_P_OR; }
		else if (p[0] == '&') { prec = 5; op = '&'; }
		else if (p[0] == '^') { prec = 4; op = '^'; }
		else if (p[0] == '|') { prec = 3; op = '|'; }
		if (prec == 0 || prec < min_prec) break;
		x->p += op >= 256 ? 2 : 1; // Multi-char punctuator codes are >= 256.
		long long rhs = cspv_ppx_binary(x, prec + 1);
		switch (op) {
		case '*': lhs *= rhs; break;
		case '/': lhs = rhs ? lhs / rhs : 0; break;
		case '%': lhs = rhs ? lhs % rhs : 0; break;
		case '+': lhs += rhs; break;
		case '-': lhs -= rhs; break;
		case CSPV_P_SHL: lhs <<= rhs; break;
		case CSPV_P_SHR: lhs >>= rhs; break;
		case '<': lhs = lhs < rhs; break;
		case '>': lhs = lhs > rhs; break;
		case CSPV_P_LE: lhs = lhs <= rhs; break;
		case CSPV_P_GE: lhs = lhs >= rhs; break;
		case CSPV_P_EQ: lhs = lhs == rhs; break;
		case CSPV_P_NE: lhs = lhs != rhs; break;
		case '&': lhs &= rhs; break;
		case '^': lhs ^= rhs; break;
		case '|': lhs |= rhs; break;
		case CSPV_P_AND: lhs = lhs && rhs; break;
		case CSPV_P_OR: lhs = lhs || rhs; break;
		}
	}
	return lhs;
}

static long long cspv_ppx_ternary(cspv_pp_expr* x)
{
	long long c = cspv_ppx_binary(x, 1);
	cspv_ppx_ws(x);
	if (*x->p == '?') {
		x->p++;
		long long a = cspv_ppx_ternary(x);
		cspv_ppx_ws(x);
		if (*x->p != ':') cspv_pp_error(x->pp, "expected ':' in #if expression");
		x->p++;
		long long b = cspv_ppx_ternary(x);
		return c ? a : b;
	}
	return c;
}

static long long cspv_pp_eval(cspv_pp* pp, const char* start, const char* end)
{
	char* expanded = cspv_pp_expand_range(pp, start, end, true);
	cspv_pp_expr x;
	x.pp = pp;
	x.p = expanded;
	long long v = cspv_ppx_ternary(&x);
	sfree(expanded);
	return v;
}

//--------------------------------------------------------------------------------------------------
// Directive + file processing.

static void cspv_pp_define(cspv_pp* pp, const char* text, const char* end)
{
	const char* p = text;
	while (p < end && (*p == ' ' || *p == '\t')) p++;
	if (p >= end || !cspv_is_ident_start(*p)) cspv_pp_error(pp, "expected macro name after #define");
	const char* ns = p;
	while (p < end && cspv_is_ident_char(*p)) p++;
	const char* name = sintern_range(ns, p);

	cspv_macro* m = (cspv_macro*)cspv_arena_alloc(&pp->ctx->arena, sizeof(cspv_macro));
	memset(m, 0, sizeof(*m));
	m->name = name;

	if (p < end && *p == '(') {
		// Function-like (no space between name and paren).
		m->is_function = true;
		p++;
		for (;;) {
			while (p < end && (*p == ' ' || *p == '\t')) p++;
			if (p < end && *p == ')') { p++; break; }
			if (p >= end || !cspv_is_ident_start(*p)) cspv_pp_error(pp, "bad parameter list for macro '%s'", name);
			const char* as = p;
			while (p < end && cspv_is_ident_char(*p)) p++;
			apush(m->params, sintern_range(as, p));
			while (p < end && (*p == ' ' || *p == '\t')) p++;
			if (p < end && *p == ',') { p++; continue; }
			if (p < end && *p == ')') { p++; break; }
			cspv_pp_error(pp, "bad parameter list for macro '%s'", name);
		}
	}

	while (p < end && (*p == ' ' || *p == '\t')) p++;
	m->body = cspv_arena_strndup(&pp->ctx->arena, p, (size_t)(end - p));
	map_set(pp->macros, (uint64_t)(uintptr_t)name, m);
}

static void cspv_pp_process_file(cspv_pp* pp, const char* source, int file_index);

// Returns true if content (and location markers) were spliced into the output;
// false when the automatic include guard skipped the file.
static bool cspv_pp_include(cspv_pp* pp, const char* text, const char* end, int parent_file, int parent_line)
{
	const char* p = text;
	while (p < end && (*p == ' ' || *p == '\t')) p++;
	char open = (p < end) ? *p : 0;
	if (open != '"' && open != '<') cspv_pp_error(pp, "expected \"path\" after #include");
	char close = open == '"' ? '"' : '>';
	p++;
	const char* ps = p;
	while (p < end && *p != close) p++;
	if (p >= end) cspv_pp_error(pp, "unterminated #include path");
	const char* path = sintern_range(ps, p);

	// Automatic include guard: each unique path is included once.
	if (map_get(pp->included, (uint64_t)(uintptr_t)path)) return false;
	map_set(pp->included, (uint64_t)(uintptr_t)path, 1);

	if (!pp->opts || !pp->opts->include_resolve) {
		cspv_pp_error(pp, "#include \"%s\" but no include resolver was provided", path);
	}
	const char* content = pp->opts->include_resolve(path, pp->opts->user);
	if (!content) cspv_pp_error(pp, "cannot open include file \"%s\"", path);

	const char* display = path;
	if (pp->opts && pp->opts->display_name) {
		const char* renamed = pp->opts->display_name(path, pp->opts->user);
		if (renamed) display = renamed;
	}
	int new_index = (int)asize(pp->ctx->files);
	apush(pp->ctx->files, display);
	cspv_pp_process_file(pp, content, new_index);

	// Restore location in the including file.
	cspv_pp_marker(pp, parent_line, parent_file);
	pp->line = parent_line;
	pp->ctx->file_index = parent_file;
	return true;
}

static void cspv_pp_process_file(cspv_pp* pp, const char* raw_source, int file_index)
{
	char* source = cspv_pp_strip_comments(pp->ctx, raw_source);
	cspv_pp_marker(pp, 1, file_index);
	pp->ctx->file_index = file_index;
	pp->line = 1;

	int cond_base = (int)asize(pp->conds);
	const char* p = source;
	while (*p) {
		// Build one logical line (join backslash-newline continuations).
		const char* line_start = p;
		int start_line = pp->line;
		CK_SDYNA char* joined = NULL;
		const char* line_end;
		for (;;) {
			line_end = line_start;
			while (*line_end && *line_end != '\n') line_end++;
			bool continued = line_end > line_start && line_end[-1] == '\\';
			if (continued || joined) {
				if (!joined) joined = smake("");
				sappend_range(joined, line_start, continued ? line_end - 1 : line_end);
			}
			if (!continued) break;
			pp->line++;
			line_start = *line_end ? line_end + 1 : line_end;
		}
		const char* ls = joined ? joined : line_start;
		const char* le = joined ? joined + slen(joined) : line_end;

		// Directive?
		const char* q = ls;
		while (q < le && (*q == ' ' || *q == '\t')) q++;
		if (q < le && *q == '#') {
			q++;
			while (q < le && (*q == ' ' || *q == '\t')) q++;
			const char* ds = q;
			while (q < le && cspv_is_ident_char(*q)) q++;
			const char* directive = sintern_range(ds, q);
			bool active = cspv_pp_active(pp);

			bool spliced_include = false;
			#define CSPV_DIRECTIVE(kw) (directive == g_cspv_pp_kw.kw)
			if (CSPV_DIRECTIVE(kw_include)) {
				if (active) spliced_include = cspv_pp_include(pp, q, le, file_index, start_line + 1);
			} else if (CSPV_DIRECTIVE(kw_define)) {
				if (active) cspv_pp_define(pp, q, le);
			} else if (CSPV_DIRECTIVE(kw_undef)) {
				if (active) {
					while (q < le && (*q == ' ' || *q == '\t')) q++;
					const char* ns = q;
					while (q < le && cspv_is_ident_char(*q)) q++;
					map_del(pp->macros, (uint64_t)(uintptr_t)sintern_range(ns, q));
				}
			} else if (CSPV_DIRECTIVE(kw_ifdef) || CSPV_DIRECTIVE(kw_ifndef)) {
				bool neg = directive == g_cspv_pp_kw.kw_ifndef;
				while (q < le && (*q == ' ' || *q == '\t')) q++;
				const char* ns = q;
				while (q < le && cspv_is_ident_char(*q)) q++;
				bool defined = map_get(pp->macros, (uint64_t)(uintptr_t)sintern_range(ns, q)) != NULL;
				cspv_pp_cond c;
				c.parent_active = active;
				c.active = active && (neg ? !defined : defined);
				c.taken = c.active;
				c.seen_else = false;
				apush(pp->conds, c);
			} else if (CSPV_DIRECTIVE(kw_if)) {
				cspv_pp_cond c;
				c.parent_active = active;
				c.active = active && cspv_pp_eval(pp, q, le) != 0;
				c.taken = c.active;
				c.seen_else = false;
				apush(pp->conds, c);
			} else if (CSPV_DIRECTIVE(kw_elif)) {
				if ((int)asize(pp->conds) <= cond_base) cspv_pp_error(pp, "#elif without #if");
				cspv_pp_cond* c = &alast(pp->conds);
				if (c->seen_else) cspv_pp_error(pp, "#elif after #else");
				if (c->taken) c->active = false;
				else c->active = c->parent_active && cspv_pp_eval(pp, q, le) != 0;
				c->taken = c->taken || c->active;
			} else if (CSPV_DIRECTIVE(kw_else)) {
				if ((int)asize(pp->conds) <= cond_base) cspv_pp_error(pp, "#else without #if");
				cspv_pp_cond* c = &alast(pp->conds);
				if (c->seen_else) cspv_pp_error(pp, "duplicate #else");
				c->seen_else = true;
				c->active = c->parent_active && !c->taken;
				c->taken = true;
			} else if (CSPV_DIRECTIVE(kw_endif)) {
				if ((int)asize(pp->conds) <= cond_base) cspv_pp_error(pp, "#endif without #if");
				apop(pp->conds);
			} else if (CSPV_DIRECTIVE(kw_version) || CSPV_DIRECTIVE(kw_extension) || CSPV_DIRECTIVE(kw_pragma) || CSPV_DIRECTIVE(kw_line)) {
				// Accepted and ignored.
			} else {
				if (active) cspv_pp_error(pp, "unknown preprocessor directive '#%s'", directive);
			}
			#undef CSPV_DIRECTIVE
			// The restore marker after a spliced include already terminates the line.
			if (!spliced_include) sappend(pp->out, "\n");
		} else if (cspv_pp_active(pp)) {
			cspv_pp_expand_into(pp, &pp->out, ls, le, false);
			sappend(pp->out, "\n");
		} else {
			sappend(pp->out, "\n");
		}

		if (joined) {
			sfree(joined);
			// Continuations consumed extra physical lines; resync the lexer.
			cspv_pp_marker(pp, pp->line + 1, file_index);
		}

		p = *line_end ? line_end + 1 : line_end;
		pp->line++;
	}

	if ((int)asize(pp->conds) != cond_base) {
		cspv_pp_error(pp, "missing #endif");
	}
}

// Entry: preprocess `source`, returning a heap (ckit) string stored on the ctx for
// cleanup. Registers builtin defines from opts.
static char* cspv_preprocess(cspv_ctx* ctx, const char* source, const CSPV_Options* opts)
{
	cspv_pp_init_keywords();

	cspv_pp pp;
	memset(&pp, 0, sizeof(pp));
	pp.ctx = ctx;
	pp.opts = opts;
	pp.out = smake("");

	apush(ctx->files, "shader");

	if (opts) {
		for (int i = 0; i < opts->num_defines; i++) {
			cspv_macro* m = (cspv_macro*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_macro));
			memset(m, 0, sizeof(*m));
			m->name = sintern(opts->defines[i].name);
			m->body = opts->defines[i].value ? cspv_arena_strndup(&ctx->arena, opts->defines[i].value, strlen(opts->defines[i].value)) : "";
			map_set(pp.macros, (uint64_t)(uintptr_t)m->name, m);
		}
	}

	cspv_pp_process_file(&pp, source, 0);

	// Free macro param arrays (macros themselves live in the arena).
	for (int i = 0; i < (int)map_size(pp.macros); i++) {
		afree(map_val(pp.macros, i)->params);
	}
	map_free(pp.macros);
	map_free(pp.included);
	afree(pp.conds);
	afree(pp.expanding);

	ctx->file_index = 0;
	return pp.out;
}

//--------------------------------------------------------------------------------------------------
// SPIR-V emission helpers.

static uint32_t cspv_new_id(cspv_ctx* ctx)
{
	return ctx->next_id++;
}

// Append one instruction to a section buffer. `words` are the operand words after the
// opcode word; the opcode word (word count << 16 | opcode) is computed here.
static void cspv_emit(CK_DYNA uint32_t** section, int opcode, const uint32_t* words, int count)
{
	apush(*section, (uint32_t)((count + 1) << 16 | opcode));
	for (int i = 0; i < count; i++) apush(*section, words[i]);
}

// Convenience emitters for fixed small operand counts.
static void cspv_emit1(CK_DYNA uint32_t** s, int op, uint32_t a) { cspv_emit(s, op, &a, 1); }
static void cspv_emit2(CK_DYNA uint32_t** s, int op, uint32_t a, uint32_t b) { uint32_t w[2] = { a, b }; cspv_emit(s, op, w, 2); }
static void cspv_emit3(CK_DYNA uint32_t** s, int op, uint32_t a, uint32_t b, uint32_t c) { uint32_t w[3] = { a, b, c }; cspv_emit(s, op, w, 3); }
static void cspv_emit4(CK_DYNA uint32_t** s, int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { uint32_t w[4] = { a, b, c, d }; cspv_emit(s, op, w, 4); }
static void cspv_emit5(CK_DYNA uint32_t** s, int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) { uint32_t w[5] = { a, b, c, d, e }; cspv_emit(s, op, w, 5); }

// Push a null-terminated string as words (SPIR-V literal string encoding).
static void cspv_push_string(CK_DYNA uint32_t** section, const char* s)
{
	size_t len = strlen(s) + 1; // Include the null terminator.
	uint32_t word = 0;
	int shift = 0;
	for (size_t i = 0; i < len; i++) {
		word |= (uint32_t)(unsigned char)s[i] << shift;
		shift += 8;
		if (shift == 32) {
			apush(*section, word);
			word = 0;
			shift = 0;
		}
	}
	if (shift != 0) apush(*section, word);
}

// Instruction with a trailing string operand (OpName, OpExtInstImport, ...).
static void cspv_emit_str(CK_DYNA uint32_t** section, int opcode, const uint32_t* words, int count, const char* s)
{
	size_t len = strlen(s) + 1;
	int str_words = (int)((len + 3) / 4);
	apush(*section, (uint32_t)((count + str_words + 1) << 16 | opcode));
	for (int i = 0; i < count; i++) apush(*section, words[i]);
	cspv_push_string(section, s);
}

static void cspv_name_id(cspv_ctx* ctx, uint32_t id, const char* name)
{
	cspv_emit_str(&ctx->names, CSpvOpName, &id, 1, name);
}

//--------------------------------------------------------------------------------------------------
// Type ids. Non-aggregate SPIR-V types must be declared exactly once, so all lookups
// go through a dedup map keyed on the canonical cspv_type pointer.

static uint32_t cspv_const_scalar(cspv_ctx* ctx, cspv_type* type, uint32_t bits);
static bool cspv_is_scalar(cspv_type* t);
static bool cspv_is_vector(cspv_type* t);
static uint32_t cspv_laid_struct_tid(cspv_ctx* ctx, cspv_type* t, int line);
static uint32_t cspv_laid_runtime_array_tid(cspv_ctx* ctx, cspv_type* elem, int line);

static uint32_t cspv_type_id(cspv_ctx* ctx, cspv_type* type)
{
	uint32_t* found = map_get_ptr(ctx->type_ids, (uint64_t)(uintptr_t)type);
	if (found) return *found;

	uint32_t id = cspv_new_id(ctx);
	switch (type->kind) {
	case CSPV_T_VOID:  cspv_emit1(&ctx->globals, CSpvOpTypeVoid, id); break;
	case CSPV_T_BOOL:  cspv_emit1(&ctx->globals, CSpvOpTypeBool, id); break;
	case CSPV_T_INT:   cspv_emit3(&ctx->globals, CSpvOpTypeInt, id, 32, 1); break;
	case CSPV_T_UINT:  cspv_emit3(&ctx->globals, CSpvOpTypeInt, id, 32, 0); break;
	case CSPV_T_FLOAT: cspv_emit2(&ctx->globals, CSpvOpTypeFloat, id, 32); break;
	default: break; // Aggregates handled below (they recurse for element types first).
	}

	if (type->kind == CSPV_T_VEC) {
		uint32_t elem = cspv_type_id(ctx, type->elem);
		cspv_emit3(&ctx->globals, CSpvOpTypeVector, id, elem, (uint32_t)type->cols);
	} else if (type->kind == CSPV_T_ARRAY && type->cols == -1) {
		// Runtime array (SSBO tail). std430 stride comes from the element type;
		// only scalar/vector elements are supported, so stride = aligned size.
		uint32_t elem = cspv_type_id(ctx, type->elem);
		cspv_emit2(&ctx->globals, CSpvOpTypeRuntimeArray, id, elem);
		int stride = 4;
		if (type->elem->kind == CSPV_T_VEC) stride = type->elem->cols == 2 ? 8 : 16;
		cspv_emit3(&ctx->decos, CSpvOpDecorate, id, CSpvDecorationArrayStride, (uint32_t)stride);
	} else if (type->kind == CSPV_T_ARRAY) {
		uint32_t elem = cspv_type_id(ctx, type->elem);
		uint32_t len = cspv_const_scalar(ctx, ctx->t_uint, (uint32_t)type->cols);
		cspv_emit3(&ctx->globals, CSpvOpTypeArray, id, elem, len);
	} else if (type->kind == CSPV_T_STRUCT) {
		CK_DYNA uint32_t* w = NULL;
		apush(w, id);
		for (int i = 0; i < (int)asize(type->field_types); i++) {
			apush(w, cspv_type_id(ctx, type->field_types[i]));
		}
		cspv_emit(&ctx->globals, CSpvOpTypeStruct, w, (int)asize(w));
		afree(w);
		cspv_name_id(ctx, id, type->name);
	} else if (type->kind == CSPV_T_MAT) {
		cspv_type* colv = ctx->t_vec[type->rows - 2];
		uint32_t col = cspv_type_id(ctx, colv);
		cspv_emit3(&ctx->globals, CSpvOpTypeMatrix, id, col, (uint32_t)type->cols);
	} else if (type->kind == CSPV_T_IMAGE2D) {
		// Storage image: sampled = 2, with an explicit format.
		bool is_uint = type->cols == 32 || type->cols == 33; // rgba8ui / r32ui
		uint32_t sampled_tid = cspv_type_id(ctx, is_uint ? ctx->t_uint : ctx->t_float);
		uint32_t w[8] = { id, sampled_tid, 1 /* 2D */, 0, 0, 0, 2 /* storage */, (uint32_t)type->cols };
		cspv_emit(&ctx->globals, CSpvOpTypeImage, w, 8);
	} else if (type->kind == CSPV_T_SAMPLER2D) {
		// Sampled type is float (sampler2D) or uint (usampler2D).
		uint32_t sampled_tid = cspv_type_id(ctx, type->elem ? type->elem : ctx->t_float);
		uint32_t img = cspv_new_id(ctx);
		// Sampled type, dim 2D (=1), depth 0, arrayed 0, ms 0, sampled 1, format Unknown (=0).
		uint32_t w[8] = { img, sampled_tid, 1, 0, 0, 0, 1, 0 };
		cspv_emit(&ctx->globals, CSpvOpTypeImage, w, 8);
		cspv_emit2(&ctx->globals, CSpvOpTypeSampledImage, id, img);
		if (type->elem && type->elem->kind == CSPV_T_UINT) ctx->usampler2d_image_tid = img;
		else ctx->sampler2d_image_tid = img;
	}

	map_set(ctx->type_ids, (uint64_t)(uintptr_t)type, id);
	return id;
}

// Pointer type to a pointee given directly by spirv id (used for laid-out buffer
// interior types that have no cspv_type of their own).
static uint32_t cspv_ptr_type_id_raw(cspv_ctx* ctx, uint32_t tid, int storage)
{
	uint64_t key = ((uint64_t)tid << 4) | (uint32_t)storage;
	uint32_t* found = map_get_ptr(ctx->ptr_type_ids, key);
	if (found) return *found;
	uint32_t id = cspv_new_id(ctx);
	cspv_emit3(&ctx->globals, CSpvOpTypePointer, id, (uint32_t)storage, tid);
	map_set(ctx->ptr_type_ids, key, id);
	return id;
}

static uint32_t cspv_ptr_type_id(cspv_ctx* ctx, cspv_type* type, int storage)
{
	return cspv_ptr_type_id_raw(ctx, cspv_type_id(ctx, type), storage);
}

//--------------------------------------------------------------------------------------------------
// Constants (deduped).

static uint32_t cspv_const_scalar(cspv_ctx* ctx, cspv_type* type, uint32_t bits)
{
	uint32_t tid = cspv_type_id(ctx, type);
	uint64_t key = ((uint64_t)tid << 32) | bits;
	uint32_t* found = map_get_ptr(ctx->const_ids, key);
	if (found) return *found;
	uint32_t id = cspv_new_id(ctx);
	if (type->kind == CSPV_T_BOOL) {
		cspv_emit2(&ctx->globals, bits ? CSpvOpConstantTrue : CSpvOpConstantFalse, tid, id);
	} else {
		cspv_emit3(&ctx->globals, CSpvOpConstant, tid, id, bits);
	}
	map_set(ctx->const_ids, key, id);
	return id;
}

static uint32_t cspv_const_float(cspv_ctx* ctx, float f)
{
	uint32_t bits;
	memcpy(&bits, &f, 4);
	return cspv_const_scalar(ctx, ctx->t_float, bits);
}

static uint32_t cspv_const_int(cspv_ctx* ctx, int32_t v)
{
	return cspv_const_scalar(ctx, ctx->t_int, (uint32_t)v);
}

static uint32_t cspv_const_uint(cspv_ctx* ctx, uint32_t v)
{
	return cspv_const_scalar(ctx, ctx->t_uint, v);
}

//--------------------------------------------------------------------------------------------------
// Values. Codegen passes these around: either an rvalue (id) or an lvalue
// (pointer id + optional swizzle applied on load/store).

typedef enum cspv_val_kind
{
	CSPV_V_RVALUE,
	CSPV_V_PTR,      // Loadable/storable pointer.
	CSPV_V_SWIZZLE,  // Swizzle applied to a pointer lvalue.
} cspv_val_kind;

typedef struct cspv_value
{
	cspv_val_kind kind;
	cspv_type* type;      // Type of the value after any swizzle.
	uint32_t id;          // CSPV_V_RVALUE: result id. Otherwise: pointer id.
	int storage;          // CSPV_V_PTR/SWIZZLE: storage class of the pointer.
	uint32_t layout_tid;  // CSPV_V_PTR into a buffer block: laid-out pointee tid (0 = plain).
	cspv_type* base_type; // CSPV_V_SWIZZLE: type behind the pointer.
	uint8_t swiz[4];      // CSPV_V_SWIZZLE: component selects.
	int swiz_count;
} cspv_value;

static cspv_value cspv_rvalue(cspv_type* type, uint32_t id)
{
	cspv_value v;
	memset(&v, 0, sizeof(v));
	v.kind = CSPV_V_RVALUE;
	v.type = type;
	v.id = id;
	return v;
}

// Load an lvalue into an rvalue id, applying any swizzle.
static uint32_t cspv_load(cspv_ctx* ctx, cspv_value* v)
{
	if (v->kind == CSPV_V_RVALUE) return v->id;
	if (v->type->kind == CSPV_T_ARRAY && v->type->cols == -1) {
		cspv_errorf(ctx, 0, "runtime arrays cannot be loaded whole; index them instead");
	}
	// Structs inside buffer blocks have laid-out (offset-decorated) types, which
	// cannot be loaded directly into plain struct values pre SPIR-V 1.4 (no
	// OpCopyLogical). Load field-by-field and reconstruct.
	if (v->kind == CSPV_V_PTR && v->layout_tid && v->type->kind == CSPV_T_STRUCT) {
		cspv_type* st = v->type;
		CK_DYNA uint32_t* w = NULL;
		uint32_t result = cspv_new_id(ctx);
		apush(w, cspv_type_id(ctx, st));
		apush(w, result);
		for (int i = 0; i < (int)asize(st->field_types); i++) {
			cspv_type* ft = st->field_types[i];
			if (!cspv_is_scalar(ft) && !cspv_is_vector(ft)) {
				cspv_errorf(ctx, 0, "cannot load '%s' whole from a buffer block; access its members instead", st->name);
			}
			uint32_t fptr = cspv_new_id(ctx);
			cspv_emit4(&ctx->body, CSpvOpAccessChain, cspv_ptr_type_id(ctx, ft, v->storage), fptr, v->id, cspv_const_int(ctx, i));
			uint32_t fval = cspv_new_id(ctx);
			cspv_emit3(&ctx->body, CSpvOpLoad, cspv_type_id(ctx, ft), fval, fptr);
			apush(w, fval);
		}
		cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, (int)asize(w));
		afree(w);
		return result;
	}
	uint32_t loaded = cspv_new_id(ctx);
	if (v->kind == CSPV_V_PTR) {
		cspv_emit3(&ctx->body, CSpvOpLoad, cspv_type_id(ctx, v->type), loaded, v->id);
		return loaded;
	}
	// Swizzle: load the whole base vector then extract/shuffle.
	cspv_emit3(&ctx->body, CSpvOpLoad, cspv_type_id(ctx, v->base_type), loaded, v->id);
	if (v->swiz_count == 1) {
		uint32_t result = cspv_new_id(ctx);
		cspv_emit4(&ctx->body, CSpvOpCompositeExtract, cspv_type_id(ctx, v->type), result, loaded, v->swiz[0]);
		return result;
	}
	uint32_t result = cspv_new_id(ctx);
	uint32_t w[4 + 4];
	int n = 0;
	w[n++] = cspv_type_id(ctx, v->type);
	w[n++] = result;
	w[n++] = loaded;
	w[n++] = loaded;
	for (int i = 0; i < v->swiz_count; i++) w[n++] = v->swiz[i];
	cspv_emit(&ctx->body, CSpvOpVectorShuffle, w, n);
	return result;
}

// Store an rvalue into an lvalue.
static void cspv_store(cspv_ctx* ctx, cspv_value* dst, uint32_t src_id, int line)
{
	if (dst->kind == CSPV_V_PTR) {
		cspv_emit2(&ctx->body, CSpvOpStore, dst->id, src_id);
		return;
	}
	if (dst->kind != CSPV_V_SWIZZLE) {
		cspv_errorf(ctx, line, "cannot assign to this expression");
	}
	// Swizzled store: load base, insert/shuffle components, store back.
	cspv_ctx* c = ctx;
	uint32_t base_tid = cspv_type_id(c, dst->base_type);
	uint32_t old_v = cspv_new_id(c);
	cspv_emit3(&c->body, CSpvOpLoad, base_tid, old_v, dst->id);
	uint32_t new_v;
	if (dst->swiz_count == 1) {
		new_v = cspv_new_id(c);
		uint32_t w[5] = { base_tid, new_v, src_id, old_v, dst->swiz[0] };
		cspv_emit(&c->body, CSpvOpCompositeInsert, w, 5);
	} else {
		// Shuffle old and new: components come from old (index i) unless selected by
		// the swizzle, in which case they come from new (index base_cols + position).
		int base_cols = dst->base_type->cols;
		uint32_t w[4 + 4];
		int n = 0;
		new_v = cspv_new_id(c);
		w[n++] = base_tid;
		w[n++] = new_v;
		w[n++] = old_v;
		w[n++] = src_id;
		for (int i = 0; i < base_cols; i++) {
			int sel = i;
			for (int j = 0; j < dst->swiz_count; j++) {
				if (dst->swiz[j] == i) {
					sel = base_cols + j;
					break;
				}
			}
			w[n++] = (uint32_t)sel;
		}
		cspv_emit(&c->body, CSpvOpVectorShuffle, w, n);
	}
	cspv_emit2(&c->body, CSpvOpStore, dst->id, new_v);
}

//--------------------------------------------------------------------------------------------------
// Scopes and symbols.

static void cspv_push_scope(cspv_ctx* ctx)
{
	cspv_scope s;
	memset(&s, 0, sizeof(s));
	apush(ctx->scopes, s);
}

static void cspv_pop_scope(cspv_ctx* ctx)
{
	cspv_scope* s = &alast(ctx->scopes);
	map_free(s->syms);
	apop(ctx->scopes);
}

static cspv_symbol* cspv_find_symbol(cspv_ctx* ctx, const char* name)
{
	for (int i = (int)asize(ctx->scopes) - 1; i >= 0; i--) {
		cspv_symbol* sym = map_get(ctx->scopes[i].syms, (uint64_t)(uintptr_t)name);
		if (sym) return sym;
	}
	return NULL;
}

static cspv_symbol* cspv_add_symbol(cspv_ctx* ctx, const char* name, cspv_sym_kind kind, cspv_type* type, uint32_t id, int storage, int line)
{
	cspv_scope* scope = &alast(ctx->scopes);
	if (map_get(scope->syms, (uint64_t)(uintptr_t)name)) {
		cspv_errorf(ctx, line, "redefinition of '%s'", name);
	}
	cspv_symbol* sym = (cspv_symbol*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_symbol));
	memset(sym, 0, sizeof(*sym));
	sym->kind = kind;
	sym->name = name;
	sym->type = type;
	sym->id = id;
	sym->storage = storage;
	map_set(scope->syms, (uint64_t)(uintptr_t)name, sym);
	return sym;
}

//--------------------------------------------------------------------------------------------------
// Interned keyword table, initialized once per compile.

typedef struct cspv_keywords
{
	const char* kw_layout;
	const char* kw_in;
	const char* kw_out;
	const char* kw_uniform;
	const char* kw_flat;
	const char* kw_const;
	const char* kw_if;
	const char* kw_else;
	const char* kw_for;
	const char* kw_while;
	const char* kw_do;
	const char* kw_switch;
	const char* kw_case;
	const char* kw_default;
	const char* kw_return;
	const char* kw_discard;
	const char* kw_break;
	const char* kw_continue;
	const char* kw_true;
	const char* kw_false;
	const char* kw_void;
	const char* kw_main;
	const char* kw_location;
	const char* kw_set;
	const char* kw_binding;
	const char* kw_gl_Position;
	const char* kw_gl_FragCoord;
	const char* kw_struct;
	const char* kw_length;
	const char* kw_buffer;
	const char* kw_readonly;
	const char* kw_writeonly;
	const char* kw_std140;
	const char* kw_std430;
	const char* kw_local_size_x;
	const char* kw_local_size_y;
	const char* kw_local_size_z;
	const char* kw_shared;
	// Storage image formats (value = SPIR-V ImageFormat enum).
	const char* kw_rgba32f;
	const char* kw_rgba16f;
	const char* kw_r32f;
	const char* kw_rgba8;
	const char* kw_rgba8ui;
	const char* kw_r32ui;
	const char* kw_rg32f;
	const char* kw_rg16f;
	const char* kw_r16f;
	const char* kw_inout;
	const char* kw_highp;
	const char* kw_mediump;
	const char* kw_lowp;
} cspv_keywords;

static cspv_keywords g_cspv_kw;

static void cspv_init_keywords(void)
{
	g_cspv_kw.kw_layout = sintern("layout");
	g_cspv_kw.kw_in = sintern("in");
	g_cspv_kw.kw_out = sintern("out");
	g_cspv_kw.kw_uniform = sintern("uniform");
	g_cspv_kw.kw_flat = sintern("flat");
	g_cspv_kw.kw_const = sintern("const");
	g_cspv_kw.kw_if = sintern("if");
	g_cspv_kw.kw_else = sintern("else");
	g_cspv_kw.kw_for = sintern("for");
	g_cspv_kw.kw_while = sintern("while");
	g_cspv_kw.kw_do = sintern("do");
	g_cspv_kw.kw_switch = sintern("switch");
	g_cspv_kw.kw_case = sintern("case");
	g_cspv_kw.kw_default = sintern("default");
	g_cspv_kw.kw_return = sintern("return");
	g_cspv_kw.kw_discard = sintern("discard");
	g_cspv_kw.kw_break = sintern("break");
	g_cspv_kw.kw_continue = sintern("continue");
	g_cspv_kw.kw_true = sintern("true");
	g_cspv_kw.kw_false = sintern("false");
	g_cspv_kw.kw_void = sintern("void");
	g_cspv_kw.kw_main = sintern("main");
	g_cspv_kw.kw_location = sintern("location");
	g_cspv_kw.kw_set = sintern("set");
	g_cspv_kw.kw_binding = sintern("binding");
	g_cspv_kw.kw_gl_Position = sintern("gl_Position");
	g_cspv_kw.kw_gl_FragCoord = sintern("gl_FragCoord");
	g_cspv_kw.kw_struct = sintern("struct");
	g_cspv_kw.kw_length = sintern("length");
	g_cspv_kw.kw_buffer = sintern("buffer");
	g_cspv_kw.kw_readonly = sintern("readonly");
	g_cspv_kw.kw_writeonly = sintern("writeonly");
	g_cspv_kw.kw_std140 = sintern("std140");
	g_cspv_kw.kw_std430 = sintern("std430");
	g_cspv_kw.kw_local_size_x = sintern("local_size_x");
	g_cspv_kw.kw_local_size_y = sintern("local_size_y");
	g_cspv_kw.kw_local_size_z = sintern("local_size_z");
	g_cspv_kw.kw_shared = sintern("shared");
	g_cspv_kw.kw_rgba32f = sintern("rgba32f");
	g_cspv_kw.kw_rgba16f = sintern("rgba16f");
	g_cspv_kw.kw_r32f = sintern("r32f");
	g_cspv_kw.kw_rgba8 = sintern("rgba8");
	g_cspv_kw.kw_rgba8ui = sintern("rgba8ui");
	g_cspv_kw.kw_r32ui = sintern("r32ui");
	g_cspv_kw.kw_rg32f = sintern("rg32f");
	g_cspv_kw.kw_rg16f = sintern("rg16f");
	g_cspv_kw.kw_r16f = sintern("r16f");
	g_cspv_kw.kw_inout = sintern("inout");
	g_cspv_kw.kw_highp = sintern("highp");
	g_cspv_kw.kw_mediump = sintern("mediump");
	g_cspv_kw.kw_lowp = sintern("lowp");
}

// Precision qualifiers are accepted and ignored (SPIR-V for Vulkan has no use for
// them; they only matter to the GLES source CF generates via SPIRV-Cross).
static bool cspv_is_precision_kw(const char* ident)
{
	return ident == g_cspv_kw.kw_highp || ident == g_cspv_kw.kw_mediump || ident == g_cspv_kw.kw_lowp;
}

//--------------------------------------------------------------------------------------------------
// Type name lookup.

static cspv_type* cspv_lookup_type(cspv_ctx* ctx, const char* name)
{
	return map_get(ctx->type_names, (uint64_t)(uintptr_t)name);
}

static bool cspv_at_type(cspv_ctx* ctx)
{
	if (ctx->tok.kind != CSPV_TOK_IDENT) return false;
	if (cspv_is_precision_kw(ctx->tok.ident)) return true;
	return cspv_lookup_type(ctx, ctx->tok.ident) != NULL;
}

static cspv_type* cspv_parse_type(cspv_ctx* ctx)
{
	while (ctx->tok.kind == CSPV_TOK_IDENT && cspv_is_precision_kw(ctx->tok.ident)) cspv_advance(ctx);
	if (ctx->tok.kind != CSPV_TOK_IDENT) cspv_errorf(ctx, ctx->tok.line, "expected type name");
	cspv_type* type = cspv_lookup_type(ctx, ctx->tok.ident);
	if (!type) cspv_errorf(ctx, ctx->tok.line, "unknown type '%s'", ctx->tok.ident);
	cspv_advance(ctx);
	return type;
}

// Canonical array type per (elem, len): array types compare by pointer like all others.
static cspv_type* cspv_array_type(cspv_ctx* ctx, cspv_type* elem, int len)
{
	uint64_t key = ((uint64_t)(uintptr_t)elem << 12) ^ (uint32_t)len;
	cspv_type* found = map_get(ctx->array_types, key);
	if (found) return found;
	cspv_type* t = (cspv_type*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_type));
	memset(t, 0, sizeof(*t));
	t->kind = CSPV_T_ARRAY;
	t->elem = elem;
	t->cols = len;
	map_set(ctx->array_types, key, t);
	return t;
}

static struct cspv_expr* cspv_parse_assign(cspv_ctx* ctx);
static void cspv_const_fold_scalar(cspv_ctx* ctx, cspv_expr* e, cspv_type** out_type, uint32_t* out_bits);

// Parse an optional `[N]` array suffix after a declarator name. N must be a positive
// integer constant expression (literals, folded arithmetic, const globals).
static cspv_type* cspv_parse_array_suffix(cspv_ctx* ctx, cspv_type* base)
{
	if (!cspv_is_punct(ctx, '[')) return base;
	if (base->kind == CSPV_T_ARRAY) cspv_errorf(ctx, ctx->tok.line, "multi-dimensional arrays are not supported");
	int line = ctx->tok.line;
	cspv_advance(ctx);
	cspv_expr* size_expr = cspv_parse_assign(ctx);
	cspv_type* t = NULL;
	uint32_t bits = 0;
	cspv_const_fold_scalar(ctx, size_expr, &t, &bits);
	if (t->kind != CSPV_T_INT && t->kind != CSPV_T_UINT) {
		cspv_errorf(ctx, line, "array size must be an integer constant expression");
	}
	int len = (int)(int32_t)bits;
	if (len <= 0) cspv_errorf(ctx, line, "array size must be positive");
	cspv_expect_punct(ctx, ']');
	if (cspv_is_punct(ctx, '[')) cspv_errorf(ctx, ctx->tok.line, "multi-dimensional arrays are not supported");
	return cspv_array_type(ctx, base, len);
}

//--------------------------------------------------------------------------------------------------
// Expression parsing (precedence climbing).

static cspv_expr* cspv_new_expr(cspv_ctx* ctx, cspv_expr_kind kind, int line)
{
	cspv_expr* e = (cspv_expr*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_expr));
	memset(e, 0, sizeof(*e));
	e->kind = kind;
	e->line = line;
	return e;
}

static cspv_expr* cspv_parse_expr(cspv_ctx* ctx);
static cspv_expr* cspv_parse_assign(cspv_ctx* ctx);

static cspv_expr* cspv_parse_primary(cspv_ctx* ctx)
{
	int line = ctx->tok.line;
	if (cspv_accept_punct(ctx, '(')) {
		cspv_expr* e = cspv_parse_expr(ctx);
		cspv_expect_punct(ctx, ')');
		return e;
	}
	if (ctx->tok.kind == CSPV_TOK_FLOAT_LIT) {
		cspv_expr* e = cspv_new_expr(ctx, CSPV_E_FLOAT_LIT, line);
		e->u.fval = ctx->tok.float_val;
		cspv_advance(ctx);
		return e;
	}
	if (ctx->tok.kind == CSPV_TOK_INT_LIT || ctx->tok.kind == CSPV_TOK_UINT_LIT) {
		cspv_expr* e = cspv_new_expr(ctx, ctx->tok.kind == CSPV_TOK_INT_LIT ? CSPV_E_INT_LIT : CSPV_E_UINT_LIT, line);
		e->u.ival = ctx->tok.int_val;
		cspv_advance(ctx);
		return e;
	}
	if (ctx->tok.kind == CSPV_TOK_IDENT) {
		const char* name = ctx->tok.ident;
		if (name == g_cspv_kw.kw_true || name == g_cspv_kw.kw_false) {
			cspv_expr* e = cspv_new_expr(ctx, CSPV_E_BOOL_LIT, line);
			e->u.bval = (name == g_cspv_kw.kw_true);
			cspv_advance(ctx);
			return e;
		}
		cspv_advance(ctx);
		// Array constructor: T[](...) or T[N](...).
		int array_size = -1;
		if (cspv_is_punct(ctx, '[') && cspv_lookup_type(ctx, name)) {
			cspv_advance(ctx);
			array_size = 0;
			if (ctx->tok.kind == CSPV_TOK_INT_LIT || ctx->tok.kind == CSPV_TOK_UINT_LIT) {
				array_size = (int)ctx->tok.int_val;
				cspv_advance(ctx);
			}
			cspv_expect_punct(ctx, ']');
		}
		if (cspv_is_punct(ctx, '(')) {
			// Function call or constructor.
			cspv_advance(ctx);
			cspv_expr* e = cspv_new_expr(ctx, CSPV_E_CALL, line);
			e->u.call.name = name;
			e->u.call.args = NULL;
			e->u.call.array_size = array_size;
			if (!cspv_is_punct(ctx, ')')) {
				do {
					cspv_expr* arg = cspv_parse_assign(ctx);
					apush(e->u.call.args, arg);
				} while (cspv_accept_punct(ctx, ','));
			}
			cspv_expect_punct(ctx, ')');
			return e;
		}
		if (array_size != -1) cspv_errorf(ctx, line, "expected '(' after array type");
		cspv_expr* e = cspv_new_expr(ctx, CSPV_E_REF, line);
		e->u.name = name;
		return e;
	}
	cspv_errorf(ctx, line, "unexpected token in expression");
	return NULL;
}

static cspv_expr* cspv_parse_postfix(cspv_ctx* ctx)
{
	cspv_expr* e = cspv_parse_primary(ctx);
	for (;;) {
		int line = ctx->tok.line;
		if (cspv_accept_punct(ctx, '.')) {
			const char* member = cspv_expect_any_ident(ctx);
			if (member == g_cspv_kw.kw_length && cspv_is_punct(ctx, '(')) {
				// arr.length()
				cspv_advance(ctx);
				cspv_expect_punct(ctx, ')');
				cspv_expr* m = cspv_new_expr(ctx, CSPV_E_LENGTH, line);
				m->u.member.base = e;
				e = m;
				continue;
			}
			cspv_expr* m = cspv_new_expr(ctx, CSPV_E_MEMBER, line);
			m->u.member.base = e;
			m->u.member.member = member;
			e = m;
		} else if (cspv_accept_punct(ctx, '[')) {
			cspv_expr* idx = cspv_new_expr(ctx, CSPV_E_INDEX, line);
			idx->u.index.base = e;
			idx->u.index.index = cspv_parse_expr(ctx);
			cspv_expect_punct(ctx, ']');
			e = idx;
		} else if (cspv_is_punct(ctx, CSPV_P_INC) || cspv_is_punct(ctx, CSPV_P_DEC)) {
			cspv_expr* u = cspv_new_expr(ctx, CSPV_E_UNARY, line);
			u->u.un.op = ctx->tok.punct;
			u->u.un.e = e;
			u->u.un.postfix = true;
			cspv_advance(ctx);
			e = u;
		} else {
			break;
		}
	}
	return e;
}

static cspv_expr* cspv_parse_unary(cspv_ctx* ctx)
{
	int line = ctx->tok.line;
	if (ctx->tok.kind == CSPV_TOK_PUNCT) {
		int p = ctx->tok.punct;
		if (p == '-' || p == '!' || p == '~' || p == '+' || p == CSPV_P_INC || p == CSPV_P_DEC) {
			cspv_advance(ctx);
			cspv_expr* operand = cspv_parse_unary(ctx);
			if (p == '+') return operand;
			cspv_expr* e = cspv_new_expr(ctx, CSPV_E_UNARY, line);
			e->u.un.op = p;
			e->u.un.e = operand;
			e->u.un.postfix = false;
			return e;
		}
	}
	return cspv_parse_postfix(ctx);
}

// Binary operator precedence. Higher binds tighter.
static int cspv_binop_prec(int punct)
{
	switch (punct) {
	case '*': case '/': case '%': return 10;
	case '+': case '-': return 9;
	case CSPV_P_SHL: case CSPV_P_SHR: return 8;
	case '<': case '>': case CSPV_P_LE: case CSPV_P_GE: return 7;
	case CSPV_P_EQ: case CSPV_P_NE: return 6;
	case '&': return 5;
	case '^': return 4;
	case '|': return 3;
	case CSPV_P_AND: return 2;
	case CSPV_P_OR: return 1;
	default: return 0;
	}
}

static cspv_expr* cspv_parse_binary(cspv_ctx* ctx, int min_prec)
{
	cspv_expr* lhs = cspv_parse_unary(ctx);
	for (;;) {
		if (ctx->tok.kind != CSPV_TOK_PUNCT) break;
		int op = ctx->tok.punct;
		int prec = cspv_binop_prec(op);
		if (prec == 0 || prec < min_prec) break;
		int line = ctx->tok.line;
		cspv_advance(ctx);
		cspv_expr* rhs = cspv_parse_binary(ctx, prec + 1);
		cspv_expr* e = cspv_new_expr(ctx, CSPV_E_BINARY, line);
		e->u.bin.op = op;
		e->u.bin.l = lhs;
		e->u.bin.r = rhs;
		lhs = e;
	}
	return lhs;
}

static cspv_expr* cspv_parse_ternary(cspv_ctx* ctx)
{
	cspv_expr* cond = cspv_parse_binary(ctx, 1);
	if (cspv_accept_punct(ctx, '?')) {
		int line = cond->line;
		cspv_expr* a = cspv_parse_assign(ctx);
		cspv_expect_punct(ctx, ':');
		cspv_expr* b = cspv_parse_assign(ctx);
		cspv_expr* e = cspv_new_expr(ctx, CSPV_E_COND, line);
		e->u.cond.c = cond;
		e->u.cond.a = a;
		e->u.cond.b = b;
		return e;
	}
	return cond;
}

static bool cspv_is_assign_op(int punct)
{
	switch (punct) {
	case '=':
	case CSPV_P_ADD_ASSIGN: case CSPV_P_SUB_ASSIGN: case CSPV_P_MUL_ASSIGN:
	case CSPV_P_DIV_ASSIGN: case CSPV_P_MOD_ASSIGN: case CSPV_P_AND_ASSIGN:
	case CSPV_P_OR_ASSIGN: case CSPV_P_XOR_ASSIGN: case CSPV_P_SHL_ASSIGN:
	case CSPV_P_SHR_ASSIGN:
		return true;
	}
	return false;
}

static cspv_expr* cspv_parse_assign(cspv_ctx* ctx)
{
	cspv_expr* lhs = cspv_parse_ternary(ctx);
	if (ctx->tok.kind == CSPV_TOK_PUNCT && cspv_is_assign_op(ctx->tok.punct)) {
		int op = ctx->tok.punct;
		int line = ctx->tok.line;
		cspv_advance(ctx);
		cspv_expr* rhs = cspv_parse_assign(ctx); // Right-associative.
		cspv_expr* e = cspv_new_expr(ctx, CSPV_E_BINARY, line);
		e->u.bin.op = op;
		e->u.bin.l = lhs;
		e->u.bin.r = rhs;
		return e;
	}
	return lhs;
}

static cspv_expr* cspv_parse_expr(cspv_ctx* ctx)
{
	// Comma operator is not supported (it never appears in CF shaders outside for-loop
	// headers, which parse their clauses individually).
	return cspv_parse_assign(ctx);
}

//--------------------------------------------------------------------------------------------------
// Statement parsing.

static cspv_stmt* cspv_new_stmt(cspv_ctx* ctx, cspv_stmt_kind kind, int line)
{
	cspv_stmt* s = (cspv_stmt*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_stmt));
	memset(s, 0, sizeof(*s));
	s->kind = kind;
	s->line = line;
	return s;
}

static cspv_stmt* cspv_parse_stmt(cspv_ctx* ctx);

static cspv_stmt* cspv_parse_block(cspv_ctx* ctx)
{
	int line = ctx->tok.line;
	cspv_expect_punct(ctx, '{');
	cspv_stmt* block = cspv_new_stmt(ctx, CSPV_S_BLOCK, line);
	while (!cspv_is_punct(ctx, '}')) {
		if (ctx->tok.kind == CSPV_TOK_EOF) cspv_errorf(ctx, ctx->tok.line, "unexpected end of file inside block");
		cspv_stmt* s = cspv_parse_stmt(ctx);
		apush(block->u.block, s);
	}
	cspv_advance(ctx); // '}'
	return block;
}

// Parse a declaration statement: `[const] type name [= init];`. The array suffix
// may sit on the type (`vec2[8] v`) or the name (`vec2 v[8]`). Assumes caller
// verified the leading type token.
static cspv_stmt* cspv_parse_decl_stmt(cspv_ctx* ctx)
{
	int line = ctx->tok.line;
	cspv_type* base_type = cspv_parse_type(ctx);
	base_type = cspv_parse_array_suffix(ctx, base_type);

	cspv_stmt* head = NULL;
	cspv_stmt* tail = NULL;
	do {
		const char* name = cspv_expect_any_ident(ctx);
		cspv_type* type = cspv_parse_array_suffix(ctx, base_type);
		cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_DECL, line);
		s->u.decl.type = type;
		s->u.decl.name = name;
		if (cspv_accept_punct(ctx, '=')) {
			s->u.decl.init = cspv_parse_assign(ctx);
		}
		if (tail) tail->u.decl.next_decl = s;
		else head = s;
		tail = s;
	} while (cspv_accept_punct(ctx, ','));
	cspv_expect_punct(ctx, ';');
	return head;
}

// Expression with the comma operator (evaluate left, result is right). Only valid
// in for-loop clauses and expression statements.
static cspv_expr* cspv_parse_comma_expr(cspv_ctx* ctx)
{
	cspv_expr* e = cspv_parse_assign(ctx);
	while (cspv_is_punct(ctx, ',')) {
		int line = ctx->tok.line;
		cspv_advance(ctx);
		cspv_expr* rhs = cspv_parse_assign(ctx);
		cspv_expr* seq = cspv_new_expr(ctx, CSPV_E_BINARY, line);
		seq->u.bin.op = ',';
		seq->u.bin.l = e;
		seq->u.bin.r = rhs;
		e = seq;
	}
	return e;
}

static cspv_stmt* cspv_parse_stmt(cspv_ctx* ctx)
{
	int line = ctx->tok.line;

	if (cspv_is_punct(ctx, '{')) return cspv_parse_block(ctx);

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_if)) {
		cspv_expect_punct(ctx, '(');
		cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_IF, line);
		s->u.if_s.cond = cspv_parse_expr(ctx);
		cspv_expect_punct(ctx, ')');
		s->u.if_s.then_s = cspv_parse_stmt(ctx);
		if (cspv_accept_ident(ctx, g_cspv_kw.kw_else)) {
			s->u.if_s.else_s = cspv_parse_stmt(ctx);
		}
		return s;
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_for)) {
		cspv_expect_punct(ctx, '(');
		cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_FOR, line);
		if (!cspv_accept_punct(ctx, ';')) {
			if (cspv_at_type(ctx)) {
				s->u.for_s.init = cspv_parse_decl_stmt(ctx); // Consumes the ';'.
			} else {
				cspv_stmt* init = cspv_new_stmt(ctx, CSPV_S_EXPR, ctx->tok.line);
				init->u.expr = cspv_parse_expr(ctx);
				s->u.for_s.init = init;
				cspv_expect_punct(ctx, ';');
			}
		}
		if (!cspv_is_punct(ctx, ';')) s->u.for_s.cond = cspv_parse_expr(ctx);
		cspv_expect_punct(ctx, ';');
		if (!cspv_is_punct(ctx, ')')) s->u.for_s.iter = cspv_parse_comma_expr(ctx);
		cspv_expect_punct(ctx, ')');
		s->u.for_s.body = cspv_parse_stmt(ctx);
		return s;
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_while)) {
		cspv_expect_punct(ctx, '(');
		cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_WHILE, line);
		s->u.while_s.cond = cspv_parse_expr(ctx);
		cspv_expect_punct(ctx, ')');
		s->u.while_s.body = cspv_parse_stmt(ctx);
		return s;
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_do)) {
		cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_DO, line);
		s->u.while_s.body = cspv_parse_stmt(ctx);
		if (!cspv_accept_ident(ctx, g_cspv_kw.kw_while)) cspv_errorf(ctx, ctx->tok.line, "expected 'while' after do-block");
		cspv_expect_punct(ctx, '(');
		s->u.while_s.cond = cspv_parse_expr(ctx);
		cspv_expect_punct(ctx, ')');
		cspv_expect_punct(ctx, ';');
		return s;
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_switch)) {
		cspv_expect_punct(ctx, '(');
		cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_SWITCH, line);
		s->u.switch_s.sel = cspv_parse_expr(ctx);
		cspv_expect_punct(ctx, ')');
		cspv_expect_punct(ctx, '{');
		cspv_switch_group* cur = NULL;
		while (!cspv_is_punct(ctx, '}')) {
			if (ctx->tok.kind == CSPV_TOK_EOF) cspv_errorf(ctx, ctx->tok.line, "unexpected end of file inside switch");
			if (cspv_accept_ident(ctx, g_cspv_kw.kw_case) || cspv_is_ident(ctx, g_cspv_kw.kw_default)) {
				bool is_default = false;
				int64_t label = 0;
				if (cspv_is_ident(ctx, g_cspv_kw.kw_default)) {
					cspv_advance(ctx);
					is_default = true;
				} else {
					// Case labels are integer literals (optionally negated).
					bool neg = cspv_accept_punct(ctx, '-');
					if (ctx->tok.kind != CSPV_TOK_INT_LIT && ctx->tok.kind != CSPV_TOK_UINT_LIT) {
						cspv_errorf(ctx, ctx->tok.line, "case label must be an integer literal");
					}
					label = (int64_t)ctx->tok.int_val;
					if (neg) label = -label;
					cspv_advance(ctx);
				}
				cspv_expect_punct(ctx, ':');
				// Consecutive labels with no statements between them share a group.
				bool start_new = true;
				if (cur && asize(cur->stmts) == 0) start_new = false;
				if (start_new) {
					cspv_switch_group g;
					memset(&g, 0, sizeof(g));
					apush(s->u.switch_s.groups, g);
					cur = &alast(s->u.switch_s.groups);
				}
				if (is_default) cur->is_default = true;
				else apush(cur->labels, label);
			} else {
				if (!cur) cspv_errorf(ctx, ctx->tok.line, "statement before first 'case' in switch");
				cspv_stmt* body_stmt = cspv_parse_stmt(ctx);
				apush(cur->stmts, body_stmt);
				cur = &alast(s->u.switch_s.groups); // Re-fetch: apush may have moved the array.
			}
		}
		cspv_advance(ctx); // '}'
		return s;
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_return)) {
		cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_RETURN, line);
		if (!cspv_is_punct(ctx, ';')) s->u.ret = cspv_parse_expr(ctx);
		cspv_expect_punct(ctx, ';');
		return s;
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_discard)) {
		cspv_expect_punct(ctx, ';');
		return cspv_new_stmt(ctx, CSPV_S_DISCARD, line);
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_break)) {
		cspv_expect_punct(ctx, ';');
		return cspv_new_stmt(ctx, CSPV_S_BREAK, line);
	}

	if (cspv_accept_ident(ctx, g_cspv_kw.kw_continue)) {
		cspv_expect_punct(ctx, ';');
		return cspv_new_stmt(ctx, CSPV_S_CONTINUE, line);
	}

	// `const` on locals is accepted and ignored (locals are Function-storage
	// OpVariables either way; downstream compilers do const propagation).
	if (cspv_accept_ident(ctx, g_cspv_kw.kw_const)) {
		if (!cspv_at_type(ctx)) cspv_errorf(ctx, ctx->tok.line, "expected type after 'const'");
		return cspv_parse_decl_stmt(ctx);
	}

	if (cspv_at_type(ctx)) {
		// Could be a declaration (`vec2 x`, `vec2[8] x`) or a constructor expression
		// statement (`vec2(...)`, `vec2[8](...)`). Distinguish by lookahead.
		if (cspv_peek(ctx, 1)->kind == CSPV_TOK_IDENT) {
			return cspv_parse_decl_stmt(ctx);
		}
		if (cspv_peek(ctx, 1)->kind == CSPV_TOK_PUNCT && cspv_peek(ctx, 1)->punct == '[' &&
		    cspv_peek(ctx, 4)->kind == CSPV_TOK_IDENT) {
			return cspv_parse_decl_stmt(ctx);
		}
	}

	cspv_stmt* s = cspv_new_stmt(ctx, CSPV_S_EXPR, line);
	s->u.expr = cspv_parse_expr(ctx);
	cspv_expect_punct(ctx, ';');
	return s;
}

//--------------------------------------------------------------------------------------------------
// Layout qualifiers.

typedef struct cspv_layout
{
	int location;
	int set;
	int binding;
	int local_size[3]; // -1 when unspecified.
	int format;        // SPIR-V ImageFormat enum, -1 when unspecified.
} cspv_layout;

static cspv_layout cspv_parse_layout(cspv_ctx* ctx)
{
	cspv_layout layout;
	layout.location = -1;
	layout.set = -1;
	layout.binding = -1;
	layout.local_size[0] = layout.local_size[1] = layout.local_size[2] = -1;
	layout.format = -1;
	cspv_expect_punct(ctx, '(');
	do {
		const char* key = cspv_expect_any_ident(ctx);
		int value = -1;
		if (cspv_accept_punct(ctx, '=')) {
			if (ctx->tok.kind != CSPV_TOK_INT_LIT && ctx->tok.kind != CSPV_TOK_UINT_LIT) {
				cspv_errorf(ctx, ctx->tok.line, "expected integer for layout qualifier '%s'", key);
			}
			value = (int)ctx->tok.int_val;
			cspv_advance(ctx);
		}
		if (key == g_cspv_kw.kw_location) layout.location = value;
		else if (key == g_cspv_kw.kw_set) layout.set = value;
		else if (key == g_cspv_kw.kw_binding) layout.binding = value;
		else if (key == g_cspv_kw.kw_std140 || key == g_cspv_kw.kw_std430) { /* Implied by block kind. */ }
		else if (key == g_cspv_kw.kw_local_size_x) layout.local_size[0] = value;
		else if (key == g_cspv_kw.kw_local_size_y) layout.local_size[1] = value;
		else if (key == g_cspv_kw.kw_local_size_z) layout.local_size[2] = value;
		else if (key == g_cspv_kw.kw_rgba32f) layout.format = 1;
		else if (key == g_cspv_kw.kw_rgba16f) layout.format = 2;
		else if (key == g_cspv_kw.kw_r32f) layout.format = 3;
		else if (key == g_cspv_kw.kw_rgba8) layout.format = 4;
		else if (key == g_cspv_kw.kw_rgba8ui) layout.format = 32;
		else if (key == g_cspv_kw.kw_r32ui) layout.format = 33;
		// Extended formats (require the StorageImageExtendedFormats capability).
		else if (key == g_cspv_kw.kw_rg32f) layout.format = 6;
		else if (key == g_cspv_kw.kw_rg16f) layout.format = 7;
		else if (key == g_cspv_kw.kw_r16f) layout.format = 9;
		else cspv_errorf(ctx, ctx->tok.line, "unsupported layout qualifier '%s'", key);
	} while (cspv_accept_punct(ctx, ','));
	cspv_expect_punct(ctx, ')');
	return layout;
}

//--------------------------------------------------------------------------------------------------
// Type utilities.

static bool cspv_is_scalar(cspv_type* t)
{
	return t->kind == CSPV_T_BOOL || t->kind == CSPV_T_INT || t->kind == CSPV_T_UINT || t->kind == CSPV_T_FLOAT;
}

static bool cspv_is_vector(cspv_type* t)
{
	return t->kind == CSPV_T_VEC;
}

// Scalar element type of a scalar or vector.
static cspv_type* cspv_elem_type(cspv_type* t)
{
	return t->kind == CSPV_T_VEC ? t->elem : t;
}

static int cspv_num_components(cspv_type* t)
{
	return t->kind == CSPV_T_VEC ? t->cols : 1;
}

// vecN type with the given scalar element type. n == 1 returns the scalar itself.
static cspv_type* cspv_vec_type(cspv_ctx* ctx, cspv_type* elem, int n)
{
	if (n == 1) return elem;
	switch (elem->kind) {
	case CSPV_T_FLOAT: return ctx->t_vec[n - 2];
	case CSPV_T_INT:   return ctx->t_ivec[n - 2];
	case CSPV_T_UINT:  return ctx->t_uvec[n - 2];
	case CSPV_T_BOOL:  return ctx->t_bvec[n - 2];
	default: return NULL;
	}
}

static const char* cspv_type_name(cspv_type* t)
{
	switch (t->kind) {
	case CSPV_T_VOID: return "void";
	case CSPV_T_BOOL: return "bool";
	case CSPV_T_INT: return "int";
	case CSPV_T_UINT: return "uint";
	case CSPV_T_FLOAT: return "float";
	case CSPV_T_SAMPLER2D: return t->elem && t->elem->kind == CSPV_T_UINT ? "usampler2D" : "sampler2D";
	case CSPV_T_ARRAY: return "array";
	case CSPV_T_STRUCT: return t->name;
	case CSPV_T_IMAGE2D: return "image2D";
	case CSPV_T_MAT:
		switch (t->cols) { case 2: return "mat2"; case 3: return "mat3"; default: return "mat4"; }
	case CSPV_T_VEC:
		switch (t->elem->kind) {
		case CSPV_T_FLOAT: switch (t->cols) { case 2: return "vec2"; case 3: return "vec3"; default: return "vec4"; }
		case CSPV_T_INT:   switch (t->cols) { case 2: return "ivec2"; case 3: return "ivec3"; default: return "ivec4"; }
		case CSPV_T_UINT:  switch (t->cols) { case 2: return "uvec2"; case 3: return "uvec3"; default: return "uvec4"; }
		default:           switch (t->cols) { case 2: return "bvec2"; case 3: return "bvec3"; default: return "bvec4"; }
		}
	}
	return "?";
}

// Rank for implicit numeric promotion: int -> uint -> float.
static int cspv_promo_rank(cspv_type_kind k)
{
	switch (k) {
	case CSPV_T_INT: return 1;
	case CSPV_T_UINT: return 2;
	case CSPV_T_FLOAT: return 3;
	default: return 0;
	}
}

//--------------------------------------------------------------------------------------------------
// Conversions and splats.

// Convert an rvalue between scalar-element types of equal component count.
// Returns the (possibly new) rvalue id. Errors if no implicit conversion exists.
static uint32_t cspv_convert(cspv_ctx* ctx, uint32_t id, cspv_type* from, cspv_type* to, int line)
{
	if (from == to) return id;
	int n_from = cspv_num_components(from);
	int n_to = cspv_num_components(to);
	cspv_type_kind fk = cspv_elem_type(from)->kind;
	cspv_type_kind tk = cspv_elem_type(to)->kind;
	if (n_from != n_to) {
		cspv_errorf(ctx, line, "cannot convert '%s' to '%s'", cspv_type_name(from), cspv_type_name(to));
	}
	if (fk == tk) return id;
	int op = 0;
	if (fk == CSPV_T_INT && tk == CSPV_T_FLOAT) op = CSpvOpConvertSToF;
	else if (fk == CSPV_T_UINT && tk == CSPV_T_FLOAT) op = CSpvOpConvertUToF;
	else if (fk == CSPV_T_INT && tk == CSPV_T_UINT) op = CSpvOpBitcast;
	else if (fk == CSPV_T_UINT && tk == CSPV_T_INT) op = CSpvOpBitcast;
	else if (fk == CSPV_T_FLOAT && tk == CSPV_T_INT) op = CSpvOpConvertFToS;  // Explicit constructor conversions only.
	else if (fk == CSPV_T_FLOAT && tk == CSPV_T_UINT) op = CSpvOpConvertFToU;
	else cspv_errorf(ctx, line, "cannot convert '%s' to '%s'", cspv_type_name(from), cspv_type_name(to));
	uint32_t result = cspv_new_id(ctx);
	cspv_emit3(&ctx->body, op, cspv_type_id(ctx, to), result, id);
	return result;
}

// True if `from` implicitly converts to `to` (per GLSL 450: int -> uint -> float,
// same component counts).
static bool cspv_implicitly_converts(cspv_type* from, cspv_type* to)
{
	if (from == to) return true;
	if (cspv_num_components(from) != cspv_num_components(to)) return false;
	if (cspv_is_scalar(from) != cspv_is_scalar(to) && !(cspv_is_vector(from) && cspv_is_vector(to))) return false;
	int fr = cspv_promo_rank(cspv_elem_type(from)->kind);
	int tr = cspv_promo_rank(cspv_elem_type(to)->kind);
	return fr != 0 && tr != 0 && fr <= tr;
}

// Splat a scalar rvalue into a vector of n copies.
static uint32_t cspv_splat(cspv_ctx* ctx, uint32_t scalar_id, cspv_type* scalar_type, int n, cspv_type** out_type)
{
	cspv_type* vt = cspv_vec_type(ctx, scalar_type, n);
	uint32_t result = cspv_new_id(ctx);
	uint32_t w[2 + 4];
	int c = 0;
	w[c++] = cspv_type_id(ctx, vt);
	w[c++] = result;
	for (int i = 0; i < n; i++) w[c++] = scalar_id;
	cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, c);
	if (out_type) *out_type = vt;
	return result;
}

//--------------------------------------------------------------------------------------------------
// Expression codegen.

static cspv_value cspv_gen_expr(cspv_ctx* ctx, cspv_expr* e);

static uint32_t cspv_gen_rvalue(cspv_ctx* ctx, cspv_expr* e, cspv_type** out_type)
{
	cspv_value v = cspv_gen_expr(ctx, e);
	if (out_type) *out_type = v.type;
	return cspv_load(ctx, &v);
}

// Spill a value into a Function-storage variable so it can be indexed with
// OpAccessChain. Returns a pointer lvalue (mutations write to the copy).
static cspv_value cspv_spill(cspv_ctx* ctx, cspv_value* v)
{
	uint32_t loaded = cspv_load(ctx, v);
	uint32_t var = cspv_new_id(ctx);
	uint32_t ptr_tid = cspv_ptr_type_id(ctx, v->type, CSpvStorageFunction);
	cspv_emit3(&ctx->local_vars, CSpvOpVariable, ptr_tid, var, CSpvStorageFunction);
	cspv_emit2(&ctx->body, CSpvOpStore, var, loaded);
	cspv_value out;
	memset(&out, 0, sizeof(out));
	out.kind = CSPV_V_PTR;
	out.type = v->type;
	out.id = var;
	out.storage = CSpvStorageFunction;
	return out;
}

// Generate an rvalue converted to an expected type.
static uint32_t cspv_gen_rvalue_as(cspv_ctx* ctx, cspv_expr* e, cspv_type* want)
{
	cspv_type* type = NULL;
	uint32_t id = cspv_gen_rvalue(ctx, e, &type);
	if (type != want && !cspv_implicitly_converts(type, want)) {
		cspv_errorf(ctx, e->line, "cannot convert '%s' to '%s'", cspv_type_name(type), cspv_type_name(want));
	}
	return cspv_convert(ctx, id, type, want, e->line);
}

// Balance two numeric operands: promote scalar element types, splat scalars against
// vectors. Outputs matching ids/types.
static void cspv_balance(cspv_ctx* ctx, int line, uint32_t* a_id, cspv_type** a_type, uint32_t* b_id, cspv_type** b_type)
{
	cspv_type* at = *a_type;
	cspv_type* bt = *b_type;
	cspv_type_kind ak = cspv_elem_type(at)->kind;
	cspv_type_kind bk = cspv_elem_type(bt)->kind;
	int ar = cspv_promo_rank(ak);
	int br = cspv_promo_rank(bk);
	if (ar == 0 || br == 0) {
		cspv_errorf(ctx, line, "invalid operands ('%s' and '%s')", cspv_type_name(at), cspv_type_name(bt));
	}

	// Promote element types to the higher rank.
	cspv_type* elem = ar >= br ? cspv_elem_type(at) : cspv_elem_type(bt);
	if (cspv_elem_type(at) != elem) {
		cspv_type* want = cspv_vec_type(ctx, elem, cspv_num_components(at));
		*a_id = cspv_convert(ctx, *a_id, at, want, line);
		at = want;
	}
	if (cspv_elem_type(bt) != elem) {
		cspv_type* want = cspv_vec_type(ctx, elem, cspv_num_components(bt));
		*b_id = cspv_convert(ctx, *b_id, bt, want, line);
		bt = want;
	}

	// Splat scalars against vectors.
	if (cspv_is_vector(at) && cspv_is_scalar(bt)) {
		*b_id = cspv_splat(ctx, *b_id, bt, at->cols, &bt);
	} else if (cspv_is_scalar(at) && cspv_is_vector(bt)) {
		*a_id = cspv_splat(ctx, *a_id, at, bt->cols, &at);
	} else if (cspv_is_vector(at) && cspv_is_vector(bt) && at->cols != bt->cols) {
		cspv_errorf(ctx, line, "vector size mismatch ('%s' and '%s')", cspv_type_name(at), cspv_type_name(bt));
	}

	*a_type = at;
	*b_type = bt;
}

// Generate an expression into discarded scratch buffers just to learn its type.
// The instructions never reach the module; ids allocated during the dry run are
// simply wasted (harmless).
static cspv_type* cspv_dry_run_type(cspv_ctx* ctx, cspv_expr* e)
{
	CK_DYNA uint32_t* saved_body = ctx->body;
	CK_DYNA uint32_t* saved_locals = ctx->local_vars;
	bool saved_terminated = ctx->block_terminated;
	ctx->body = NULL;
	ctx->local_vars = NULL;
	cspv_value v = cspv_gen_expr(ctx, e);
	afree(ctx->body);
	afree(ctx->local_vars);
	ctx->body = saved_body;
	ctx->local_vars = saved_locals;
	ctx->block_terminated = saved_terminated;
	return v.type;
}

// The common type two operands promote to (int -> uint -> float, scalars splat
// against vectors), without emitting any conversion code.
static cspv_type* cspv_unify_types(cspv_ctx* ctx, cspv_type* at, cspv_type* bt, int line)
{
	if (at == bt) return at;
	int ar = cspv_promo_rank(cspv_elem_type(at)->kind);
	int br = cspv_promo_rank(cspv_elem_type(bt)->kind);
	if (ar == 0 || br == 0) {
		cspv_errorf(ctx, line, "mismatched types ('%s' and '%s')", cspv_type_name(at), cspv_type_name(bt));
	}
	cspv_type* elem = ar >= br ? cspv_elem_type(at) : cspv_elem_type(bt);
	int cols_a = cspv_num_components(at);
	int cols_b = cspv_num_components(bt);
	if (cspv_is_vector(at) && cspv_is_vector(bt) && cols_a != cols_b) {
		cspv_errorf(ctx, line, "vector size mismatch ('%s' and '%s')", cspv_type_name(at), cspv_type_name(bt));
	}
	return cspv_vec_type(ctx, elem, cols_a > cols_b ? cols_a : cols_b);
}

// Emit an arithmetic/bitwise/comparison binary op on balanced operands.
static cspv_value cspv_gen_binop(cspv_ctx* ctx, int op, int line, uint32_t a, cspv_type* at, uint32_t b, cspv_type* bt)
{
	// Matrix operations (square matrices only; '*' is the only supported operator).
	if (at->kind == CSPV_T_MAT || bt->kind == CSPV_T_MAT) {
		if (op != '*') {
			cspv_errorf(ctx, line, "only '*' is supported with matrix operands");
		}
		uint32_t result = cspv_new_id(ctx);
		if (at->kind == CSPV_T_MAT && bt->kind == CSPV_T_MAT) {
			if (at != bt) cspv_errorf(ctx, line, "matrix size mismatch");
			cspv_emit4(&ctx->body, CSpvOpMatrixTimesMatrix, cspv_type_id(ctx, at), result, a, b);
			return cspv_rvalue(at, result);
		}
		if (at->kind == CSPV_T_MAT && cspv_is_vector(bt)) {
			if (bt->elem->kind != CSPV_T_FLOAT || bt->cols != at->cols) cspv_errorf(ctx, line, "matrix/vector size mismatch");
			cspv_type* rt = ctx->t_vec[at->rows - 2];
			cspv_emit4(&ctx->body, CSpvOpMatrixTimesVector, cspv_type_id(ctx, rt), result, a, b);
			return cspv_rvalue(rt, result);
		}
		if (cspv_is_vector(at) && bt->kind == CSPV_T_MAT) {
			if (at->elem->kind != CSPV_T_FLOAT || at->cols != bt->rows) cspv_errorf(ctx, line, "vector/matrix size mismatch");
			cspv_type* rt = ctx->t_vec[bt->cols - 2];
			cspv_emit4(&ctx->body, CSpvOpVectorTimesMatrix, cspv_type_id(ctx, rt), result, a, b);
			return cspv_rvalue(rt, result);
		}
		// Matrix * scalar (either order).
		uint32_t mat = at->kind == CSPV_T_MAT ? a : b;
		cspv_type* mat_type = at->kind == CSPV_T_MAT ? at : bt;
		uint32_t scalar = at->kind == CSPV_T_MAT ? b : a;
		cspv_type* scalar_type = at->kind == CSPV_T_MAT ? bt : at;
		if (!cspv_is_scalar(scalar_type)) cspv_errorf(ctx, line, "invalid matrix operand");
		scalar = cspv_convert(ctx, scalar, scalar_type, ctx->t_float, line);
		cspv_emit4(&ctx->body, CSpvOpMatrixTimesScalar, cspv_type_id(ctx, mat_type), result, mat, scalar);
		return cspv_rvalue(mat_type, result);
	}

	cspv_balance(ctx, line, &a, &at, &b, &bt);
	cspv_type_kind ek = cspv_elem_type(at)->kind;
	bool is_float = ek == CSPV_T_FLOAT;
	bool is_signed = ek == CSPV_T_INT;

	int opcode = 0;
	cspv_type* result_type = at;
	switch (op) {
	case '+': opcode = is_float ? CSpvOpFAdd : CSpvOpIAdd; break;
	case '-': opcode = is_float ? CSpvOpFSub : CSpvOpISub; break;
	case '*': opcode = is_float ? CSpvOpFMul : CSpvOpIMul; break;
	case '/': opcode = is_float ? CSpvOpFDiv : (is_signed ? CSpvOpSDiv : CSpvOpUDiv); break;
	case '%': opcode = is_float ? CSpvOpFMod : (is_signed ? CSpvOpSMod : CSpvOpUMod); break;
	case '&': opcode = CSpvOpBitwiseAnd; break;
	case '|': opcode = CSpvOpBitwiseOr; break;
	case '^': opcode = CSpvOpBitwiseXor; break;
	case CSPV_P_SHL: opcode = CSpvOpShiftLeftLogical; break;
	case CSPV_P_SHR: opcode = is_signed ? CSpvOpShiftRightArithmetic : CSpvOpShiftRightLogical; break;
	case '<': opcode = is_float ? CSpvOpFOrdLessThan : (is_signed ? CSpvOpSLessThan : CSpvOpULessThan); result_type = ctx->t_bool; break;
	case '>': opcode = is_float ? CSpvOpFOrdGreaterThan : (is_signed ? CSpvOpSGreaterThan : CSpvOpUGreaterThan); result_type = ctx->t_bool; break;
	case CSPV_P_LE: opcode = is_float ? CSpvOpFOrdLessThanEqual : (is_signed ? CSpvOpSLessThanEqual : CSpvOpULessThanEqual); result_type = ctx->t_bool; break;
	case CSPV_P_GE: opcode = is_float ? CSpvOpFOrdGreaterThanEqual : (is_signed ? CSpvOpSGreaterThanEqual : CSpvOpUGreaterThanEqual); result_type = ctx->t_bool; break;
	case CSPV_P_EQ: opcode = is_float ? CSpvOpFOrdEqual : CSpvOpIEqual; result_type = ctx->t_bool; break;
	case CSPV_P_NE: opcode = is_float ? CSpvOpFUnordNotEqual : CSpvOpINotEqual; result_type = ctx->t_bool; break;
	default:
		cspv_errorf(ctx, line, "unsupported binary operator");
	}

	if (result_type == ctx->t_bool && cspv_is_vector(at)) {
		cspv_errorf(ctx, line, "comparison operators require scalar operands (use lessThan()/equal() for vectors)");
	}
	if ((op == '&' || op == '|' || op == '^' || op == CSPV_P_SHL || op == CSPV_P_SHR) && is_float) {
		cspv_errorf(ctx, line, "bitwise operators require integer operands");
	}

	uint32_t result = cspv_new_id(ctx);
	cspv_emit4(&ctx->body, opcode, cspv_type_id(ctx, result_type), result, a, b);
	return cspv_rvalue(result_type, result);
}

// Map a compound-assignment punct to its base binary op ('+' for +=, etc.).
static int cspv_compound_base_op(int punct)
{
	switch (punct) {
	case CSPV_P_ADD_ASSIGN: return '+';
	case CSPV_P_SUB_ASSIGN: return '-';
	case CSPV_P_MUL_ASSIGN: return '*';
	case CSPV_P_DIV_ASSIGN: return '/';
	case CSPV_P_MOD_ASSIGN: return '%';
	case CSPV_P_AND_ASSIGN: return '&';
	case CSPV_P_OR_ASSIGN: return '|';
	case CSPV_P_XOR_ASSIGN: return '^';
	case CSPV_P_SHL_ASSIGN: return CSPV_P_SHL;
	case CSPV_P_SHR_ASSIGN: return CSPV_P_SHR;
	default: return 0;
	}
}

//--------------------------------------------------------------------------------------------------
// Swizzles.

static bool cspv_swizzle_indices(const char* s, uint8_t* out, int* count)
{
	int n = (int)strlen(s);
	if (n < 1 || n > 4) return false;
	for (int i = 0; i < n; i++) {
		int idx;
		switch (s[i]) {
		case 'x': case 'r': case 's': idx = 0; break;
		case 'y': case 'g': case 't': idx = 1; break;
		case 'z': case 'b': case 'p': idx = 2; break;
		case 'w': case 'a': case 'q': idx = 3; break;
		default: return false;
		}
		out[i] = (uint8_t)idx;
	}
	*count = n;
	return true;
}

static cspv_value cspv_gen_member(cspv_ctx* ctx, cspv_expr* e)
{
	cspv_value base = cspv_gen_expr(ctx, e->u.member.base);

	// Struct field access.
	if (base.type->kind == CSPV_T_STRUCT) {
		cspv_type* st = base.type;
		int field = -1;
		for (int i = 0; i < (int)asize(st->field_names); i++) {
			if (st->field_names[i] == e->u.member.member) { field = i; break; }
		}
		if (field < 0) cspv_errorf(ctx, e->line, "'%s' has no field '%s'", st->name, e->u.member.member);
		cspv_type* ft = st->field_types[field];
		if (base.kind == CSPV_V_PTR) {
			// Inside a buffer block, struct/struct-array fields keep their laid-out
			// pointee types; scalar/vector fields use the shared types.
			uint32_t field_laid = 0;
			if (base.layout_tid) {
				if (ft->kind == CSPV_T_STRUCT) field_laid = cspv_laid_struct_tid(ctx, ft, e->line);
				else if (ft->kind == CSPV_T_ARRAY && ft->cols == -1 && ft->elem->kind == CSPV_T_STRUCT) field_laid = cspv_laid_runtime_array_tid(ctx, ft->elem, e->line);
			}
			uint32_t ptr_tid = field_laid ? cspv_ptr_type_id_raw(ctx, field_laid, base.storage)
			                              : cspv_ptr_type_id(ctx, ft, base.storage);
			uint32_t ptr = cspv_new_id(ctx);
			uint32_t index = cspv_const_int(ctx, field);
			cspv_emit4(&ctx->body, CSpvOpAccessChain, ptr_tid, ptr, base.id, index);
			cspv_value v;
			memset(&v, 0, sizeof(v));
			v.kind = CSPV_V_PTR;
			v.type = ft;
			v.id = ptr;
			v.storage = base.storage;
			v.layout_tid = field_laid;
			return v;
		}
		uint32_t loaded = cspv_load(ctx, &base);
		uint32_t result = cspv_new_id(ctx);
		cspv_emit4(&ctx->body, CSpvOpCompositeExtract, cspv_type_id(ctx, ft), result, loaded, (uint32_t)field);
		return cspv_rvalue(ft, result);
	}

	if (!cspv_is_vector(base.type)) {
		cspv_errorf(ctx, e->line, "'%s' is not a member of '%s'", e->u.member.member, cspv_type_name(base.type));
	}
	uint8_t swiz[4];
	int count = 0;
	if (!cspv_swizzle_indices(e->u.member.member, swiz, &count)) {
		cspv_errorf(ctx, e->line, "invalid swizzle '.%s'", e->u.member.member);
	}
	for (int i = 0; i < count; i++) {
		if (swiz[i] >= base.type->cols) {
			cspv_errorf(ctx, e->line, "swizzle '.%s' out of range for '%s'", e->u.member.member, cspv_type_name(base.type));
		}
	}
	cspv_type* result_type = cspv_vec_type(ctx, base.type->elem, count);

	if (base.kind == CSPV_V_PTR) {
		// Single components become component pointers (works for atomics too);
		// multi-component swizzles stay load/shuffle/store lvalues.
		if (count == 1) {
			uint32_t ptr = cspv_new_id(ctx);
			uint32_t ptr_tid = cspv_ptr_type_id(ctx, base.type->elem, base.storage);
			uint32_t index = cspv_const_int(ctx, swiz[0]);
			cspv_emit4(&ctx->body, CSpvOpAccessChain, ptr_tid, ptr, base.id, index);
			cspv_value v;
			memset(&v, 0, sizeof(v));
			v.kind = CSPV_V_PTR;
			v.type = base.type->elem;
			v.id = ptr;
			v.storage = base.storage;
			return v;
		}
		// Keep it an lvalue so it can be stored through.
		cspv_value v;
		memset(&v, 0, sizeof(v));
		v.kind = CSPV_V_SWIZZLE;
		v.type = result_type;
		v.id = base.id;
		v.storage = base.storage;
		v.base_type = base.type;
		memcpy(v.swiz, swiz, sizeof(swiz));
		v.swiz_count = count;
		return v;
	}

	// rvalue (or nested swizzle): load and extract/shuffle.
	uint32_t loaded = cspv_load(ctx, &base);
	uint32_t result = cspv_new_id(ctx);
	if (count == 1) {
		cspv_emit4(&ctx->body, CSpvOpCompositeExtract, cspv_type_id(ctx, result_type), result, loaded, swiz[0]);
	} else {
		uint32_t w[4 + 4];
		int n = 0;
		w[n++] = cspv_type_id(ctx, result_type);
		w[n++] = result;
		w[n++] = loaded;
		w[n++] = loaded;
		for (int i = 0; i < count; i++) w[n++] = swiz[i];
		cspv_emit(&ctx->body, CSpvOpVectorShuffle, w, n);
	}
	return cspv_rvalue(result_type, result);
}

//--------------------------------------------------------------------------------------------------
// References.

static cspv_value cspv_gen_ref(cspv_ctx* ctx, cspv_expr* e)
{
	cspv_symbol* sym = cspv_find_symbol(ctx, e->u.name);
	if (!sym) cspv_errorf(ctx, e->line, "undeclared identifier '%s'", e->u.name);

	cspv_value v;
	memset(&v, 0, sizeof(v));
	switch (sym->kind) {
	case CSPV_SYM_VAR:
		v.kind = CSPV_V_PTR;
		v.type = sym->type;
		v.id = sym->id;
		v.storage = sym->storage;
		// Named block instances point at the block's laid-out struct.
		if (sym->type->kind == CSPV_T_STRUCT &&
		    (sym->storage == CSpvStorageStorageBuffer || sym->storage == CSpvStorageUniform)) {
			v.layout_tid = cspv_type_id(ctx, sym->type);
		}
		return v;
	case CSPV_SYM_BLOCK_MEMBER: {
		uint32_t ptr = cspv_new_id(ctx);
		uint32_t ptr_tid = sym->laid_tid ? cspv_ptr_type_id_raw(ctx, sym->laid_tid, sym->storage)
		                                 : cspv_ptr_type_id(ctx, sym->type, sym->storage);
		uint32_t index = cspv_const_int(ctx, sym->member_index);
		cspv_emit4(&ctx->body, CSpvOpAccessChain, ptr_tid, ptr, sym->id, index);
		v.kind = CSPV_V_PTR;
		v.type = sym->type;
		v.id = ptr;
		v.storage = sym->storage;
		v.layout_tid = sym->laid_tid;
		return v;
	}
	case CSPV_SYM_GL_POSITION: {
		uint32_t ptr = cspv_new_id(ctx);
		uint32_t ptr_tid = cspv_ptr_type_id(ctx, ctx->t_vec[2], CSpvStorageOutput);
		uint32_t index = cspv_const_int(ctx, 0);
		cspv_emit4(&ctx->body, CSpvOpAccessChain, ptr_tid, ptr, ctx->gl_pervertex_var, index);
		v.kind = CSPV_V_PTR;
		v.type = ctx->t_vec[2]; // vec4
		v.id = ptr;
		v.storage = CSpvStorageOutput;
		return v;
	}
	default:
		cspv_errorf(ctx, e->line, "'%s' is a function", e->u.name);
	}
	return v;
}

//--------------------------------------------------------------------------------------------------
// Constructors: vecN(...), matN(...), float(x), etc.

static cspv_value cspv_gen_constructor(cspv_ctx* ctx, cspv_expr* e, cspv_type* type)
{
	int argc = (int)asize(e->u.call.args);
	if (argc == 0) cspv_errorf(ctx, e->line, "constructor '%s' requires arguments", cspv_type_name(type));

	if (cspv_is_scalar(type)) {
		// Scalar conversion: float(x), int(x), uint(x), bool(x).
		if (argc != 1) cspv_errorf(ctx, e->line, "scalar constructor takes exactly one argument");
		cspv_type* at = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[0], &at);
		if (cspv_is_vector(at)) {
			// Take the first component.
			uint32_t x = cspv_new_id(ctx);
			cspv_emit4(&ctx->body, CSpvOpCompositeExtract, cspv_type_id(ctx, at->elem), x, a, 0);
			a = x;
			at = at->elem;
		}
		return cspv_rvalue(type, cspv_convert(ctx, a, at, type, e->line));
	}

	if (type->kind == CSPV_T_MAT) {
		// matN(scalar): diagonal. matN(columns or N*N scalars): column-major fill.
		int rows = type->rows;
		int cols = type->cols;
		uint32_t comps[16];
		int n = 0;
		if (argc == 1) {
			cspv_type* at = NULL;
			uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[0], &at);
			if (!cspv_is_scalar(at)) cspv_errorf(ctx, e->line, "cannot construct '%s' from '%s'", cspv_type_name(type), cspv_type_name(at));
			uint32_t d = cspv_convert(ctx, a, at, ctx->t_float, e->line);
			uint32_t zero = cspv_const_float(ctx, 0.0f);
			for (int c = 0; c < cols; c++) {
				for (int r = 0; r < rows; r++) {
					comps[n++] = (c == r) ? d : zero;
				}
			}
		} else {
			for (int i = 0; i < argc; i++) {
				cspv_type* at = NULL;
				uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[i], &at);
				if (cspv_is_scalar(at)) {
					if (n >= 16) cspv_errorf(ctx, e->line, "too many components for '%s'", cspv_type_name(type));
					comps[n++] = cspv_convert(ctx, a, at, ctx->t_float, e->line);
				} else if (cspv_is_vector(at)) {
					for (int j = 0; j < at->cols; j++) {
						if (n >= 16) cspv_errorf(ctx, e->line, "too many components for '%s'", cspv_type_name(type));
						uint32_t x = cspv_new_id(ctx);
						cspv_emit4(&ctx->body, CSpvOpCompositeExtract, cspv_type_id(ctx, at->elem), x, a, (uint32_t)j);
						comps[n++] = cspv_convert(ctx, x, at->elem, ctx->t_float, e->line);
					}
				} else {
					cspv_errorf(ctx, e->line, "invalid constructor argument of type '%s'", cspv_type_name(at));
				}
			}
			if (n != rows * cols) {
				cspv_errorf(ctx, e->line, "'%s' constructor requires %d components, got %d", cspv_type_name(type), rows * cols, n);
			}
		}
		// Build columns, then the matrix.
		cspv_type* col_type = ctx->t_vec[rows - 2];
		uint32_t col_tid = cspv_type_id(ctx, col_type);
		uint32_t col_ids[4];
		for (int c = 0; c < cols; c++) {
			uint32_t w[2 + 4];
			int cw = 0;
			uint32_t cid = cspv_new_id(ctx);
			w[cw++] = col_tid;
			w[cw++] = cid;
			for (int r = 0; r < rows; r++) w[cw++] = comps[c * rows + r];
			cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, cw);
			col_ids[c] = cid;
		}
		uint32_t result = cspv_new_id(ctx);
		uint32_t w[2 + 4];
		int cw = 0;
		w[cw++] = cspv_type_id(ctx, type);
		w[cw++] = result;
		for (int c = 0; c < cols; c++) w[cw++] = col_ids[c];
		cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, cw);
		return cspv_rvalue(type, result);
	}

	if (type->kind == CSPV_T_STRUCT) {
		if (argc != (int)asize(type->field_types)) {
			cspv_errorf(ctx, e->line, "'%s' constructor requires %d argument(s), got %d", type->name, (int)asize(type->field_types), argc);
		}
		CK_DYNA uint32_t* w = NULL;
		apush(w, cspv_type_id(ctx, type));
		uint32_t result = cspv_new_id(ctx);
		apush(w, result);
		for (int i = 0; i < argc; i++) {
			apush(w, cspv_gen_rvalue_as(ctx, e->u.call.args[i], type->field_types[i]));
		}
		cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, (int)asize(w));
		afree(w);
		return cspv_rvalue(type, result);
	}

	if (!cspv_is_vector(type)) {
		cspv_errorf(ctx, e->line, "unsupported constructor '%s'", cspv_type_name(type));
	}

	// Vector constructor. Flatten arguments into converted components.
	cspv_type* elem = type->elem;
	uint32_t comps[4];
	int n = 0;

	// Single scalar arg: splat.
	if (argc == 1) {
		cspv_type* at = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[0], &at);
		if (cspv_is_scalar(at)) {
			uint32_t conv = cspv_convert(ctx, a, at, elem, e->line);
			cspv_type* vt = NULL;
			uint32_t id = cspv_splat(ctx, conv, elem, type->cols, &vt);
			return cspv_rvalue(type, id);
		}
		if (cspv_is_vector(at) && at->cols >= type->cols) {
			// vecN(vecM) with M >= N: shrink (e.g. vec3(v4) or same-size element conversion).
			for (int i = 0; i < type->cols; i++) {
				uint32_t x = cspv_new_id(ctx);
				cspv_emit4(&ctx->body, CSpvOpCompositeExtract, cspv_type_id(ctx, at->elem), x, a, (uint32_t)i);
				comps[n++] = cspv_convert(ctx, x, at->elem, elem, e->line);
			}
			uint32_t result = cspv_new_id(ctx);
			uint32_t w[2 + 4];
			int c = 0;
			w[c++] = cspv_type_id(ctx, type);
			w[c++] = result;
			for (int i = 0; i < n; i++) w[c++] = comps[i];
			cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, c);
			return cspv_rvalue(type, result);
		}
		cspv_errorf(ctx, e->line, "cannot construct '%s' from '%s'", cspv_type_name(type), cspv_type_name(at));
	}

	// Multiple args: flatten components in order.
	for (int i = 0; i < argc; i++) {
		cspv_type* at = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[i], &at);
		if (cspv_is_scalar(at)) {
			if (n >= 4) cspv_errorf(ctx, e->line, "too many components for '%s'", cspv_type_name(type));
			comps[n++] = cspv_convert(ctx, a, at, elem, e->line);
		} else if (cspv_is_vector(at)) {
			for (int j = 0; j < at->cols; j++) {
				if (n >= 4) cspv_errorf(ctx, e->line, "too many components for '%s'", cspv_type_name(type));
				uint32_t x = cspv_new_id(ctx);
				cspv_emit4(&ctx->body, CSpvOpCompositeExtract, cspv_type_id(ctx, at->elem), x, a, (uint32_t)j);
				comps[n++] = cspv_convert(ctx, x, at->elem, elem, e->line);
			}
		} else {
			cspv_errorf(ctx, e->line, "invalid constructor argument of type '%s'", cspv_type_name(at));
		}
	}
	if (n != type->cols) {
		cspv_errorf(ctx, e->line, "'%s' constructor requires %d components, got %d", cspv_type_name(type), type->cols, n);
	}
	uint32_t result = cspv_new_id(ctx);
	uint32_t w[2 + 4];
	int c = 0;
	w[c++] = cspv_type_id(ctx, type);
	w[c++] = result;
	for (int i = 0; i < n; i++) w[c++] = comps[i];
	cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, c);
	return cspv_rvalue(type, result);
}

//--------------------------------------------------------------------------------------------------
// Intrinsics.

typedef enum cspv_intrin_kind
{
	CSPV_INTRIN_EXT,        // GLSL.std.450 genType op; float by default, int/uint via op_s/op_u.
	CSPV_INTRIN_EXT_RET_F,  // GLSL.std.450, result is scalar float (length, distance).
	CSPV_INTRIN_CORE_UNARY, // Core opcode, one operand, result = operand type (fwidth, dFdx...).
	CSPV_INTRIN_DOT,
	CSPV_INTRIN_TEXTURE,        // texture(sampler, uv)
	CSPV_INTRIN_TEXTURE_OFFSET, // textureOffset(sampler, uv, const ivec2)
	CSPV_INTRIN_TEXTURE_LOD,  // textureLod(sampler, uv, lod)
	CSPV_INTRIN_TEXEL_FETCH,  // texelFetch(sampler, ivec2, lod)
	CSPV_INTRIN_TEXTURE_SIZE, // textureSize(sampler, lod) -> ivec2
	CSPV_INTRIN_RELATIONAL,   // lessThan & friends: (vecN, vecN) -> bvecN.
	CSPV_INTRIN_BOOL_REDUCE,  // all/any: bvecN -> bool.
	CSPV_INTRIN_BOOL_NOT,     // not(bvecN) -> bvecN.
	CSPV_INTRIN_FLOAT_TEST,   // isnan/isinf: float genType -> bool genType.
	CSPV_INTRIN_BITCAST,      // floatBitsToInt etc.: op is the target scalar kind.
	CSPV_INTRIN_PACK,         // pack/unpack fixed signatures (see cspv_pack_shape).
	CSPV_INTRIN_MATRIX,       // transpose/inverse/determinant.
	CSPV_INTRIN_BARRIER,      // barrier/memoryBarrier* family; op selects the flavor.
	CSPV_INTRIN_ATOMIC,       // atomic*; op = opcode (or op_s/op_u for min/max), shape 1 = compSwap.
	CSPV_INTRIN_IMAGE,        // imageLoad/imageStore/imageSize; op: 0 load, 1 store, 2 size.
} cspv_intrin_kind;

typedef enum cspv_pack_shape
{
	CSPV_PACK_V4_TO_U,
	CSPV_PACK_V2_TO_U,
	CSPV_PACK_U_TO_V4,
	CSPV_PACK_U_TO_V2,
} cspv_pack_shape;

typedef struct cspv_intrin
{
	cspv_intrin_kind kind;
	int op;    // Ext inst number, core opcode, or kind-specific payload.
	int op_s;  // CSPV_INTRIN_EXT/RELATIONAL: signed-int variant (0 = float only).
	int op_u;  // CSPV_INTRIN_EXT/RELATIONAL: unsigned variant.
	int nargs; // -1: 1 or 2 args (atan).
	int shape; // CSPV_INTRIN_PACK: cspv_pack_shape.
} cspv_intrin;

static CK_MAP(cspv_intrin) g_cspv_intrins;

static void cspv_add_intrin_full(const char* name, cspv_intrin_kind kind, int op, int op_s, int op_u, int nargs, int shape)
{
	cspv_intrin in;
	in.kind = kind;
	in.op = op;
	in.op_s = op_s;
	in.op_u = op_u;
	in.nargs = nargs;
	in.shape = shape;
	map_set(g_cspv_intrins, (uint64_t)(uintptr_t)sintern(name), in);
}

static void cspv_add_intrin(const char* name, cspv_intrin_kind kind, int op, int nargs)
{
	cspv_add_intrin_full(name, kind, op, 0, 0, nargs, 0);
}

static void cspv_init_intrins(void)
{
	if (g_cspv_intrins) return;
	cspv_add_intrin("radians", CSPV_INTRIN_EXT, 11, 1);
	cspv_add_intrin("degrees", CSPV_INTRIN_EXT, 12, 1);
	cspv_add_intrin("sin", CSPV_INTRIN_EXT, GLSLstd450Sin, 1);
	cspv_add_intrin("cos", CSPV_INTRIN_EXT, GLSLstd450Cos, 1);
	cspv_add_intrin("tan", CSPV_INTRIN_EXT, GLSLstd450Tan, 1);
	cspv_add_intrin("asin", CSPV_INTRIN_EXT, GLSLstd450Asin, 1);
	cspv_add_intrin("acos", CSPV_INTRIN_EXT, GLSLstd450Acos, 1);
	cspv_add_intrin("atan", CSPV_INTRIN_EXT, GLSLstd450Atan, -1); // atan(y_over_x) or atan(y, x).
	cspv_add_intrin("pow", CSPV_INTRIN_EXT, GLSLstd450Pow, 2);
	cspv_add_intrin("exp", CSPV_INTRIN_EXT, GLSLstd450Exp, 1);
	cspv_add_intrin("log", CSPV_INTRIN_EXT, GLSLstd450Log, 1);
	cspv_add_intrin("exp2", CSPV_INTRIN_EXT, GLSLstd450Exp2, 1);
	cspv_add_intrin("log2", CSPV_INTRIN_EXT, GLSLstd450Log2, 1);
	cspv_add_intrin("sqrt", CSPV_INTRIN_EXT, GLSLstd450Sqrt, 1);
	cspv_add_intrin("inversesqrt", CSPV_INTRIN_EXT, GLSLstd450InverseSqrt, 1);
	cspv_add_intrin_full("abs", CSPV_INTRIN_EXT, GLSLstd450FAbs, GLSLstd450SAbs, GLSLstd450SAbs, 1, 0);
	cspv_add_intrin_full("sign", CSPV_INTRIN_EXT, GLSLstd450FSign, GLSLstd450SSign, GLSLstd450SSign, 1, 0);
	cspv_add_intrin("floor", CSPV_INTRIN_EXT, GLSLstd450Floor, 1);
	cspv_add_intrin("ceil", CSPV_INTRIN_EXT, GLSLstd450Ceil, 1);
	cspv_add_intrin("fract", CSPV_INTRIN_EXT, GLSLstd450Fract, 1);
	cspv_add_intrin("round", CSPV_INTRIN_EXT, GLSLstd450Round, 1);
	cspv_add_intrin("trunc", CSPV_INTRIN_EXT, GLSLstd450Trunc, 1);
	cspv_add_intrin_full("min", CSPV_INTRIN_EXT, GLSLstd450FMin, GLSLstd450SMin, GLSLstd450UMin, 2, 0);
	cspv_add_intrin_full("max", CSPV_INTRIN_EXT, GLSLstd450FMax, GLSLstd450SMax, GLSLstd450UMax, 2, 0);
	cspv_add_intrin_full("clamp", CSPV_INTRIN_EXT, GLSLstd450FClamp, GLSLstd450SClamp, GLSLstd450UClamp, 3, 0);
	cspv_add_intrin("mix", CSPV_INTRIN_EXT, GLSLstd450FMix, 3);
	cspv_add_intrin("step", CSPV_INTRIN_EXT, GLSLstd450Step, 2);
	cspv_add_intrin("smoothstep", CSPV_INTRIN_EXT, GLSLstd450SmoothStep, 3);
	cspv_add_intrin("mod", CSPV_INTRIN_EXT, -1, 2); // Handled specially: core OpFMod.
	cspv_add_intrin("normalize", CSPV_INTRIN_EXT, GLSLstd450Normalize, 1);
	cspv_add_intrin("cross", CSPV_INTRIN_EXT, GLSLstd450Cross, 2);
	cspv_add_intrin("reflect", CSPV_INTRIN_EXT, GLSLstd450Reflect, 2);
	cspv_add_intrin("refract", CSPV_INTRIN_EXT, GLSLstd450Refract, 3);
	cspv_add_intrin("length", CSPV_INTRIN_EXT_RET_F, GLSLstd450Length, 1);
	cspv_add_intrin("distance", CSPV_INTRIN_EXT_RET_F, GLSLstd450Distance, 2);
	cspv_add_intrin("dot", CSPV_INTRIN_DOT, CSpvOpDot, 2);
	cspv_add_intrin("dFdx", CSPV_INTRIN_CORE_UNARY, CSpvOpDPdx, 1);
	cspv_add_intrin("dFdy", CSPV_INTRIN_CORE_UNARY, CSpvOpDPdy, 1);
	cspv_add_intrin("fwidth", CSPV_INTRIN_CORE_UNARY, CSpvOpFwidth, 1);
	cspv_add_intrin("texture", CSPV_INTRIN_TEXTURE, 0, 2);
	cspv_add_intrin("textureOffset", CSPV_INTRIN_TEXTURE_OFFSET, 0, 3);
	cspv_add_intrin("textureLod", CSPV_INTRIN_TEXTURE_LOD, 0, 3);
	cspv_add_intrin("texelFetch", CSPV_INTRIN_TEXEL_FETCH, 0, 3);
	cspv_add_intrin("textureSize", CSPV_INTRIN_TEXTURE_SIZE, 0, 2);
	cspv_add_intrin_full("lessThan", CSPV_INTRIN_RELATIONAL, CSpvOpFOrdLessThan, CSpvOpSLessThan, CSpvOpULessThan, 2, 0);
	cspv_add_intrin_full("lessThanEqual", CSPV_INTRIN_RELATIONAL, CSpvOpFOrdLessThanEqual, CSpvOpSLessThanEqual, CSpvOpULessThanEqual, 2, 0);
	cspv_add_intrin_full("greaterThan", CSPV_INTRIN_RELATIONAL, CSpvOpFOrdGreaterThan, CSpvOpSGreaterThan, CSpvOpUGreaterThan, 2, 0);
	cspv_add_intrin_full("greaterThanEqual", CSPV_INTRIN_RELATIONAL, CSpvOpFOrdGreaterThanEqual, CSpvOpSGreaterThanEqual, CSpvOpUGreaterThanEqual, 2, 0);
	cspv_add_intrin_full("equal", CSPV_INTRIN_RELATIONAL, CSpvOpFOrdEqual, CSpvOpIEqual, CSpvOpIEqual, 2, 0);
	cspv_add_intrin_full("notEqual", CSPV_INTRIN_RELATIONAL, CSpvOpFUnordNotEqual, CSpvOpINotEqual, CSpvOpINotEqual, 2, 0);
	cspv_add_intrin("all", CSPV_INTRIN_BOOL_REDUCE, CSpvOpAll, 1);
	cspv_add_intrin("any", CSPV_INTRIN_BOOL_REDUCE, CSpvOpAny, 1);
	cspv_add_intrin("not", CSPV_INTRIN_BOOL_NOT, CSpvOpLogicalNot, 1);
	cspv_add_intrin("isnan", CSPV_INTRIN_FLOAT_TEST, 156, 1); // OpIsNan
	cspv_add_intrin("isinf", CSPV_INTRIN_FLOAT_TEST, 157, 1); // OpIsInf
	cspv_add_intrin("floatBitsToInt", CSPV_INTRIN_BITCAST, CSPV_T_INT, 1);
	cspv_add_intrin("floatBitsToUint", CSPV_INTRIN_BITCAST, CSPV_T_UINT, 1);
	cspv_add_intrin("intBitsToFloat", CSPV_INTRIN_BITCAST, CSPV_T_FLOAT, 1);
	cspv_add_intrin("uintBitsToFloat", CSPV_INTRIN_BITCAST, CSPV_T_FLOAT, 1);
	cspv_add_intrin_full("packUnorm4x8", CSPV_INTRIN_PACK, 55, 0, 0, 1, CSPV_PACK_V4_TO_U);
	cspv_add_intrin_full("packSnorm4x8", CSPV_INTRIN_PACK, 54, 0, 0, 1, CSPV_PACK_V4_TO_U);
	cspv_add_intrin_full("packUnorm2x16", CSPV_INTRIN_PACK, 57, 0, 0, 1, CSPV_PACK_V2_TO_U);
	cspv_add_intrin_full("packSnorm2x16", CSPV_INTRIN_PACK, 56, 0, 0, 1, CSPV_PACK_V2_TO_U);
	cspv_add_intrin_full("packHalf2x16", CSPV_INTRIN_PACK, 58, 0, 0, 1, CSPV_PACK_V2_TO_U);
	cspv_add_intrin_full("unpackUnorm4x8", CSPV_INTRIN_PACK, 64, 0, 0, 1, CSPV_PACK_U_TO_V4);
	cspv_add_intrin_full("unpackSnorm4x8", CSPV_INTRIN_PACK, 63, 0, 0, 1, CSPV_PACK_U_TO_V4);
	cspv_add_intrin_full("unpackUnorm2x16", CSPV_INTRIN_PACK, 61, 0, 0, 1, CSPV_PACK_U_TO_V2);
	cspv_add_intrin_full("unpackSnorm2x16", CSPV_INTRIN_PACK, 60, 0, 0, 1, CSPV_PACK_U_TO_V2);
	cspv_add_intrin_full("unpackHalf2x16", CSPV_INTRIN_PACK, 62, 0, 0, 1, CSPV_PACK_U_TO_V2);
	cspv_add_intrin_full("transpose", CSPV_INTRIN_MATRIX, -1, 0, 0, 1, 0);
	cspv_add_intrin_full("inverse", CSPV_INTRIN_MATRIX, GLSLstd450MatrixInverse, 0, 0, 1, 0);
	cspv_add_intrin_full("determinant", CSPV_INTRIN_MATRIX, GLSLstd450Determinant, 0, 0, 1, 0);
	cspv_add_intrin("barrier", CSPV_INTRIN_BARRIER, 0, 0);
	cspv_add_intrin("memoryBarrier", CSPV_INTRIN_BARRIER, 1, 0);
	cspv_add_intrin("memoryBarrierShared", CSPV_INTRIN_BARRIER, 2, 0);
	cspv_add_intrin("memoryBarrierBuffer", CSPV_INTRIN_BARRIER, 3, 0);
	cspv_add_intrin("groupMemoryBarrier", CSPV_INTRIN_BARRIER, 4, 0);
	cspv_add_intrin("atomicAdd", CSPV_INTRIN_ATOMIC, CSpvOpAtomicIAdd, 2);
	cspv_add_intrin_full("atomicMin", CSPV_INTRIN_ATOMIC, 0, CSpvOpAtomicSMin, CSpvOpAtomicUMin, 2, 0);
	cspv_add_intrin_full("atomicMax", CSPV_INTRIN_ATOMIC, 0, CSpvOpAtomicSMax, CSpvOpAtomicUMax, 2, 0);
	cspv_add_intrin("atomicAnd", CSPV_INTRIN_ATOMIC, CSpvOpAtomicAnd, 2);
	cspv_add_intrin("atomicOr", CSPV_INTRIN_ATOMIC, CSpvOpAtomicOr, 2);
	cspv_add_intrin("atomicXor", CSPV_INTRIN_ATOMIC, CSpvOpAtomicXor, 2);
	cspv_add_intrin("atomicExchange", CSPV_INTRIN_ATOMIC, CSpvOpAtomicExchange, 2);
	cspv_add_intrin_full("atomicCompSwap", CSPV_INTRIN_ATOMIC, CSpvOpAtomicCompareExchange, 0, 0, 3, 1);
	cspv_add_intrin("imageLoad", CSPV_INTRIN_IMAGE, 0, 2);
	cspv_add_intrin("imageStore", CSPV_INTRIN_IMAGE, 1, 3);
	cspv_add_intrin("imageSize", CSPV_INTRIN_IMAGE, 2, 1);
}

static uint32_t cspv_const_expr(cspv_ctx* ctx, cspv_expr* e, cspv_type* want);
static void cspv_begin_block(cspv_ctx* ctx, uint32_t label_id);
static void cspv_branch(cspv_ctx* ctx, uint32_t target);

// Emit a single ExtInst.
static uint32_t cspv_ext_inst(cspv_ctx* ctx, cspv_type* result_type, int inst, const uint32_t* args, int nargs)
{
	uint32_t result = cspv_new_id(ctx);
	uint32_t w[4 + 3];
	int n = 0;
	w[n++] = cspv_type_id(ctx, result_type);
	w[n++] = result;
	w[n++] = ctx->glsl_ext_id;
	w[n++] = (uint32_t)inst;
	for (int i = 0; i < nargs; i++) w[n++] = args[i];
	cspv_emit(&ctx->body, CSpvOpExtInst, w, n);
	return result;
}

static cspv_value cspv_gen_intrin(cspv_ctx* ctx, cspv_expr* e, cspv_intrin* in)
{
	int argc = (int)asize(e->u.call.args);
	if (in->nargs >= 0 && argc != in->nargs) {
		cspv_errorf(ctx, e->line, "'%s' expects %d argument(s), got %d", e->u.call.name, in->nargs, argc);
	}
	if (in->nargs == -1 && argc != 1 && argc != 2) {
		cspv_errorf(ctx, e->line, "'%s' expects 1 or 2 arguments, got %d", e->u.call.name, argc);
	}

	// Sampler-based intrinsics.
	if (in->kind == CSPV_INTRIN_TEXTURE || in->kind == CSPV_INTRIN_TEXTURE_OFFSET ||
	    in->kind == CSPV_INTRIN_TEXTURE_LOD ||
	    in->kind == CSPV_INTRIN_TEXEL_FETCH || in->kind == CSPV_INTRIN_TEXTURE_SIZE) {
		cspv_type* st = NULL;
		uint32_t sampler = cspv_gen_rvalue(ctx, e->u.call.args[0], &st);
		if (st->kind != CSPV_T_SAMPLER2D) cspv_errorf(ctx, e->line, "'%s' requires a sampler2D", e->u.call.name);
		bool uint_sampler = st->elem && st->elem->kind == CSPV_T_UINT;
		cspv_type* vec4_t = uint_sampler ? ctx->t_uvec[2] : ctx->t_vec[2];

		if (in->kind == CSPV_INTRIN_TEXTURE) {
			uint32_t coord = cspv_gen_rvalue_as(ctx, e->u.call.args[1], ctx->t_vec[0]);
			uint32_t result = cspv_new_id(ctx);
			if (ctx->stage == CSPV_STAGE_FRAGMENT) {
				cspv_emit4(&ctx->body, CSpvOpImageSampleImplicitLod, cspv_type_id(ctx, vec4_t), result, sampler, coord);
			} else {
				// Outside fragment shaders an explicit LOD is required.
				uint32_t lod = cspv_const_float(ctx, 0.0f);
				uint32_t w[6] = { cspv_type_id(ctx, vec4_t), result, sampler, coord, 0x2 /* Lod */, lod };
				cspv_emit(&ctx->body, CSpvOpImageSampleExplicitLod, w, 6);
			}
			return cspv_rvalue(vec4_t, result);
		}

		if (in->kind == CSPV_INTRIN_TEXTURE_OFFSET) {
			// The offset must be a constant expression (SPIR-V ConstOffset operand).
			uint32_t coord = cspv_gen_rvalue_as(ctx, e->u.call.args[1], ctx->t_vec[0]);
			uint32_t offset = cspv_const_expr(ctx, e->u.call.args[2], ctx->t_ivec[0]);
			uint32_t result = cspv_new_id(ctx);
			if (ctx->stage == CSPV_STAGE_FRAGMENT) {
				uint32_t w[6] = { cspv_type_id(ctx, vec4_t), result, sampler, coord, 0x8 /* ConstOffset */, offset };
				cspv_emit(&ctx->body, CSpvOpImageSampleImplicitLod, w, 6);
			} else {
				uint32_t lod = cspv_const_float(ctx, 0.0f);
				uint32_t w[8] = { cspv_type_id(ctx, vec4_t), result, sampler, coord, 0x2 | 0x8 /* Lod|ConstOffset */, lod, offset };
				cspv_emit(&ctx->body, CSpvOpImageSampleExplicitLod, w, 7);
			}
			return cspv_rvalue(vec4_t, result);
		}

		if (in->kind == CSPV_INTRIN_TEXTURE_LOD) {
			uint32_t coord = cspv_gen_rvalue_as(ctx, e->u.call.args[1], ctx->t_vec[0]);
			uint32_t lod = cspv_gen_rvalue_as(ctx, e->u.call.args[2], ctx->t_float);
			uint32_t result = cspv_new_id(ctx);
			uint32_t w[6] = { cspv_type_id(ctx, vec4_t), result, sampler, coord, 0x2 /* Lod */, lod };
			cspv_emit(&ctx->body, CSpvOpImageSampleExplicitLod, w, 6);
			return cspv_rvalue(vec4_t, result);
		}

		// texelFetch/textureSize operate on the image, not the sampled image.
		uint32_t image = cspv_new_id(ctx);
		cspv_emit3(&ctx->body, CSpvOpImage, uint_sampler ? ctx->usampler2d_image_tid : ctx->sampler2d_image_tid, image, sampler);

		if (in->kind == CSPV_INTRIN_TEXEL_FETCH) {
			uint32_t coord = cspv_gen_rvalue_as(ctx, e->u.call.args[1], ctx->t_ivec[0]);
			uint32_t lod = cspv_gen_rvalue_as(ctx, e->u.call.args[2], ctx->t_int);
			uint32_t result = cspv_new_id(ctx);
			uint32_t w[6] = { cspv_type_id(ctx, vec4_t), result, image, coord, 0x2 /* Lod */, lod };
			cspv_emit(&ctx->body, CSpvOpImageFetch, w, 6);
			return cspv_rvalue(vec4_t, result);
		}

		// textureSize.
		ctx->needs_image_query = true;
		uint32_t lod = cspv_gen_rvalue_as(ctx, e->u.call.args[1], ctx->t_int);
		uint32_t result = cspv_new_id(ctx);
		cspv_emit4(&ctx->body, CSpvOpImageQuerySizeLod, cspv_type_id(ctx, ctx->t_ivec[0]), result, image, lod);
		return cspv_rvalue(ctx->t_ivec[0], result);
	}

	if (in->kind == CSPV_INTRIN_BARRIER) {
		if (ctx->stage != CSPV_STAGE_COMPUTE) cspv_errorf(ctx, e->line, "'%s' requires a compute shader", e->u.call.name);
		// Scope Workgroup = 2, Device = 1. Semantics: AcquireRelease | memory class.
		uint32_t wg = cspv_const_uint(ctx, 2);
		uint32_t dev = cspv_const_uint(ctx, 1);
		switch (in->op) {
		case 0: cspv_emit3(&ctx->body, CSpvOpControlBarrier, wg, wg, cspv_const_uint(ctx, 0x108)); break; // barrier()
		case 1: cspv_emit2(&ctx->body, CSpvOpMemoryBarrier, dev, cspv_const_uint(ctx, 0x948)); break;     // memoryBarrier()
		case 2: cspv_emit2(&ctx->body, CSpvOpMemoryBarrier, dev, cspv_const_uint(ctx, 0x108)); break;     // memoryBarrierShared()
		case 3: cspv_emit2(&ctx->body, CSpvOpMemoryBarrier, dev, cspv_const_uint(ctx, 0x48)); break;      // memoryBarrierBuffer()
		case 4: cspv_emit2(&ctx->body, CSpvOpMemoryBarrier, wg, cspv_const_uint(ctx, 0x948)); break;      // groupMemoryBarrier()
		}
		return cspv_rvalue(ctx->t_void, 0);
	}

	if (in->kind == CSPV_INTRIN_ATOMIC) {
		cspv_value mem = cspv_gen_expr(ctx, e->u.call.args[0]);
		if (mem.kind != CSPV_V_PTR || (mem.type != ctx->t_int && mem.type != ctx->t_uint)) {
			cspv_errorf(ctx, e->line, "'%s' requires an int or uint buffer/shared variable", e->u.call.name);
		}
		bool is_uint = mem.type == ctx->t_uint;
		int opcode = in->op ? in->op : (is_uint ? in->op_u : in->op_s);
		uint32_t scope = cspv_const_uint(ctx, 1);     // Device
		uint32_t semantics = cspv_const_uint(ctx, 0); // Relaxed
		uint32_t result = cspv_new_id(ctx);
		uint32_t tid = cspv_type_id(ctx, mem.type);
		if (in->shape == 1) {
			// atomicCompSwap(mem, compare, value)
			uint32_t cmp = cspv_gen_rvalue_as(ctx, e->u.call.args[1], mem.type);
			uint32_t val = cspv_gen_rvalue_as(ctx, e->u.call.args[2], mem.type);
			uint32_t w[8] = { tid, result, mem.id, scope, semantics, semantics, val, cmp };
			cspv_emit(&ctx->body, opcode, w, 8);
		} else {
			uint32_t val = cspv_gen_rvalue_as(ctx, e->u.call.args[1], mem.type);
			uint32_t w[6] = { tid, result, mem.id, scope, semantics, val };
			cspv_emit(&ctx->body, opcode, w, 6);
		}
		return cspv_rvalue(mem.type, result);
	}

	if (in->kind == CSPV_INTRIN_IMAGE) {
		cspv_type* it = NULL;
		uint32_t img = cspv_gen_rvalue(ctx, e->u.call.args[0], &it);
		if (it->kind != CSPV_T_IMAGE2D) cspv_errorf(ctx, e->line, "'%s' requires an image2D", e->u.call.name);
		bool is_uint = it->cols == 32 || it->cols == 33;
		cspv_type* texel_type = is_uint ? ctx->t_uvec[2] : ctx->t_vec[2];
		if (in->op == 2) { // imageSize
			ctx->needs_image_query = true;
			uint32_t result = cspv_new_id(ctx);
			cspv_emit3(&ctx->body, CSpvOpImageQuerySize, cspv_type_id(ctx, ctx->t_ivec[0]), result, img);
			return cspv_rvalue(ctx->t_ivec[0], result);
		}
		uint32_t coord = cspv_gen_rvalue_as(ctx, e->u.call.args[1], ctx->t_ivec[0]);
		if (in->op == 0) { // imageLoad
			uint32_t result = cspv_new_id(ctx);
			cspv_emit4(&ctx->body, CSpvOpImageRead, cspv_type_id(ctx, texel_type), result, img, coord);
			return cspv_rvalue(texel_type, result);
		}
		uint32_t texel = cspv_gen_rvalue_as(ctx, e->u.call.args[2], texel_type); // imageStore
		cspv_emit3(&ctx->body, CSpvOpImageWrite, img, coord, texel);
		return cspv_rvalue(ctx->t_void, 0);
	}

	if (in->kind == CSPV_INTRIN_MATRIX) {
		cspv_type* mt = NULL;
		uint32_t m = cspv_gen_rvalue(ctx, e->u.call.args[0], &mt);
		if (mt->kind != CSPV_T_MAT) cspv_errorf(ctx, e->line, "'%s' requires a matrix", e->u.call.name);
		if (in->op == -1) { // transpose (square matrices: same type).
			uint32_t result = cspv_new_id(ctx);
			cspv_emit3(&ctx->body, CSpvOpTranspose, cspv_type_id(ctx, mt), result, m);
			return cspv_rvalue(mt, result);
		}
		cspv_type* rt = (in->op == GLSLstd450Determinant) ? ctx->t_float : mt;
		return cspv_rvalue(rt, cspv_ext_inst(ctx, rt, in->op, &m, 1));
	}

	if (in->kind == CSPV_INTRIN_BITCAST) {
		cspv_type* at = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[0], &at);
		if (at->kind == CSPV_T_MAT || cspv_elem_type(at)->kind == CSPV_T_BOOL) {
			cspv_errorf(ctx, e->line, "invalid operand for '%s'", e->u.call.name);
		}
		cspv_type* target_elem =
			in->op == CSPV_T_INT ? ctx->t_int :
			in->op == CSPV_T_UINT ? ctx->t_uint : ctx->t_float;
		cspv_type* rt = cspv_vec_type(ctx, target_elem, cspv_num_components(at));
		uint32_t result = cspv_new_id(ctx);
		cspv_emit3(&ctx->body, CSpvOpBitcast, cspv_type_id(ctx, rt), result, a);
		return cspv_rvalue(rt, result);
	}

	if (in->kind == CSPV_INTRIN_PACK) {
		cspv_type* want =
			in->shape == CSPV_PACK_V4_TO_U ? ctx->t_vec[2] :
			in->shape == CSPV_PACK_V2_TO_U ? ctx->t_vec[0] : ctx->t_uint;
		cspv_type* rt =
			in->shape == CSPV_PACK_U_TO_V4 ? ctx->t_vec[2] :
			in->shape == CSPV_PACK_U_TO_V2 ? ctx->t_vec[0] : ctx->t_uint;
		uint32_t a = cspv_gen_rvalue_as(ctx, e->u.call.args[0], want);
		return cspv_rvalue(rt, cspv_ext_inst(ctx, rt, in->op, &a, 1));
	}

	if (in->kind == CSPV_INTRIN_BOOL_REDUCE || in->kind == CSPV_INTRIN_BOOL_NOT) {
		cspv_type* at = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[0], &at);
		if (!cspv_is_vector(at) || at->elem != ctx->t_bool) {
			cspv_errorf(ctx, e->line, "'%s' requires a bvec argument", e->u.call.name);
		}
		cspv_type* rt = in->kind == CSPV_INTRIN_BOOL_NOT ? at : ctx->t_bool;
		uint32_t result = cspv_new_id(ctx);
		cspv_emit3(&ctx->body, in->op, cspv_type_id(ctx, rt), result, a);
		return cspv_rvalue(rt, result);
	}

	if (in->kind == CSPV_INTRIN_FLOAT_TEST) {
		cspv_type* at = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.call.args[0], &at);
		if (cspv_elem_type(at)->kind != CSPV_T_FLOAT) cspv_errorf(ctx, e->line, "'%s' requires float operands", e->u.call.name);
		cspv_type* rt = cspv_vec_type(ctx, ctx->t_bool, cspv_num_components(at));
		uint32_t result = cspv_new_id(ctx);
		cspv_emit3(&ctx->body, in->op, cspv_type_id(ctx, rt), result, a);
		return cspv_rvalue(rt, result);
	}

	// Generic genType handling: evaluate, then balance all args to a common
	// scalar/vector type (scalars splat against the widest vector argument).
	uint32_t ids[3];
	cspv_type* types[3];
	for (int i = 0; i < argc; i++) {
		ids[i] = cspv_gen_rvalue(ctx, e->u.call.args[i], &types[i]);
	}

	// mix(x, y, bvec) selects components: OpSelect.
	if (in->kind == CSPV_INTRIN_EXT && in->op == GLSLstd450FMix && argc == 3 &&
	    cspv_elem_type(types[2])->kind == CSPV_T_BOOL) {
		if (types[0] != types[1] || cspv_num_components(types[2]) != cspv_num_components(types[0])) {
			cspv_errorf(ctx, e->line, "mix() with a bvec selector requires matching operand types");
		}
		uint32_t result = cspv_new_id(ctx);
		cspv_emit5(&ctx->body, CSpvOpSelect, cspv_type_id(ctx, types[0]), result, ids[2], ids[1], ids[0]);
		return cspv_rvalue(types[0], result);
	}

	int target_cols = 1;
	for (int i = 0; i < argc; i++) {
		int n = cspv_num_components(types[i]);
		if (n > target_cols) target_cols = n;
	}

	// Pick the element domain: float unless every argument is an integer and the
	// intrinsic has integer variants.
	bool all_int = in->op_s != 0;
	for (int i = 0; i < argc && all_int; i++) {
		cspv_type_kind k = cspv_elem_type(types[i])->kind;
		if (k != CSPV_T_INT && k != CSPV_T_UINT) all_int = false;
	}
	cspv_type* elem = ctx->t_float;
	int inst = in->op;
	if (all_int) {
		bool any_uint = false;
		for (int i = 0; i < argc; i++) {
			if (cspv_elem_type(types[i])->kind == CSPV_T_UINT) any_uint = true;
		}
		elem = any_uint ? ctx->t_uint : ctx->t_int;
		inst = any_uint ? in->op_u : in->op_s;
	}
	cspv_type* target = cspv_vec_type(ctx, elem, target_cols);

	for (int i = 0; i < argc; i++) {
		cspv_type* ft = cspv_vec_type(ctx, elem, cspv_num_components(types[i]));
		if (!cspv_implicitly_converts(types[i], ft) && !(cspv_elem_type(types[i])->kind == CSPV_T_UINT && elem->kind == CSPV_T_INT)) {
			cspv_errorf(ctx, e->line, "invalid argument type '%s' in call to '%s'", cspv_type_name(types[i]), e->u.call.name);
		}
		ids[i] = cspv_convert(ctx, ids[i], types[i], ft, e->line);
		types[i] = ft;
		if (cspv_num_components(types[i]) == 1 && target_cols > 1) {
			ids[i] = cspv_splat(ctx, ids[i], elem, target_cols, &types[i]);
		} else if (cspv_num_components(types[i]) != target_cols) {
			cspv_errorf(ctx, e->line, "vector size mismatch in call to '%s'", e->u.call.name);
		}
	}

	if (in->kind == CSPV_INTRIN_RELATIONAL) {
		cspv_type* rt = cspv_vec_type(ctx, ctx->t_bool, target_cols);
		int opcode = elem->kind == CSPV_T_FLOAT ? in->op : (elem->kind == CSPV_T_UINT ? in->op_u : in->op_s);
		uint32_t result = cspv_new_id(ctx);
		cspv_emit4(&ctx->body, opcode, cspv_type_id(ctx, rt), result, ids[0], ids[1]);
		return cspv_rvalue(rt, result);
	}

	if (in->op == -1 && in->kind == CSPV_INTRIN_EXT) {
		// mod(x, y) -> core OpFMod.
		uint32_t result = cspv_new_id(ctx);
		cspv_emit4(&ctx->body, CSpvOpFMod, cspv_type_id(ctx, target), result, ids[0], ids[1]);
		return cspv_rvalue(target, result);
	}

	if (in->kind == CSPV_INTRIN_DOT) {
		uint32_t result = cspv_new_id(ctx);
		cspv_emit4(&ctx->body, CSpvOpDot, cspv_type_id(ctx, ctx->t_float), result, ids[0], ids[1]);
		return cspv_rvalue(ctx->t_float, result);
	}

	if (in->kind == CSPV_INTRIN_CORE_UNARY) {
		uint32_t result = cspv_new_id(ctx);
		cspv_emit3(&ctx->body, in->op, cspv_type_id(ctx, target), result, ids[0]);
		return cspv_rvalue(target, result);
	}

	// atan with two args is atan2.
	if (in->nargs == -1 && argc == 2) inst = GLSLstd450Atan2;

	cspv_type* result_type = (in->kind == CSPV_INTRIN_EXT_RET_F) ? ctx->t_float : target;
	return cspv_rvalue(result_type, cspv_ext_inst(ctx, result_type, inst, ids, argc));
}

//--------------------------------------------------------------------------------------------------
// Calls (dispatch to constructor / user function / intrinsic).

static cspv_value cspv_gen_call(cspv_ctx* ctx, cspv_expr* e)
{
	const char* name = e->u.call.name;
	int argc = (int)asize(e->u.call.args);

	// Array constructor: T[](...) / T[N](...).
	if (e->u.call.array_size != -1) {
		cspv_type* elem = cspv_lookup_type(ctx, name);
		if (!elem) cspv_errorf(ctx, e->line, "unknown type '%s'", name);
		int len = e->u.call.array_size ? e->u.call.array_size : argc;
		if (argc != len) cspv_errorf(ctx, e->line, "array constructor expects %d element(s), got %d", len, argc);
		if (len <= 0) cspv_errorf(ctx, e->line, "array constructor requires at least one element");
		cspv_type* at = cspv_array_type(ctx, elem, len);
		CK_DYNA uint32_t* w = NULL;
		apush(w, cspv_type_id(ctx, at));
		uint32_t result = cspv_new_id(ctx);
		apush(w, result);
		for (int i = 0; i < argc; i++) {
			apush(w, cspv_gen_rvalue_as(ctx, e->u.call.args[i], elem));
		}
		cspv_emit(&ctx->body, CSpvOpCompositeConstruct, w, (int)asize(w));
		afree(w);
		return cspv_rvalue(at, result);
	}

	// Constructor?
	cspv_type* ctor = cspv_lookup_type(ctx, name);
	if (ctor) return cspv_gen_constructor(ctx, e, ctor);

	// User function? (Users may shadow builtins.)
	cspv_symbol* fn = map_get(ctx->functions, (uint64_t)(uintptr_t)name);
	if (fn) {
		// Overload resolution: prefer exact match, else first implicitly-convertible.
		cspv_symbol* exact = NULL;
		cspv_symbol* convertible = NULL;

		// Evaluate args once, keeping lvalues (needed for out/inout).
		CK_DYNA cspv_value* vals = NULL;
		for (int i = 0; i < argc; i++) {
			apush(vals, cspv_gen_expr(ctx, e->u.call.args[i]));
		}

		for (cspv_symbol* o = fn; o; o = o->next_overload) {
			if ((int)asize(o->params) != argc) continue;
			bool all_exact = true;
			bool all_conv = true;
			for (int i = 0; i < argc; i++) {
				if (o->params[i] != vals[i].type) all_exact = false;
				bool conv_ok = o->param_quals && o->param_quals[i]
					? o->params[i] == vals[i].type // out/inout: exact type only.
					: cspv_implicitly_converts(vals[i].type, o->params[i]);
				if (!conv_ok) all_conv = false;
			}
			if (all_exact) { exact = o; break; }
			if (all_conv && !convertible) convertible = o;
		}
		cspv_symbol* chosen = exact ? exact : convertible;
		if (!chosen) {
			afree(vals);
			cspv_errorf(ctx, e->line, "no matching overload for '%s'", name);
		}

		// out/inout arguments use glslang's copy-in/copy-out protocol: a Function
		// temp pointer is passed, then copied back into the argument lvalue.
		typedef struct { uint32_t temp; int arg; } cspv_copy_out;
		CK_DYNA cspv_copy_out* copy_outs = NULL;

		// 3 header words (type, result, callee) + up to 16 call arguments.
		uint32_t w[3 + 16];
		int n = 0;
		uint32_t result = cspv_new_id(ctx);
		w[n++] = cspv_type_id(ctx, chosen->type);
		w[n++] = result;
		w[n++] = chosen->id;
		for (int i = 0; i < argc; i++) {
			if (n >= (int)(sizeof(w) / sizeof(w[0]))) cspv_errorf(ctx, e->line, "too many arguments");
			int qual = chosen->param_quals ? chosen->param_quals[i] : 0;
			if (qual) {
				if (vals[i].kind == CSPV_V_RVALUE) {
					cspv_errorf(ctx, e->line, "argument %d of '%s' must be assignable (out/inout parameter)", i + 1, name);
				}
				uint32_t temp = cspv_new_id(ctx);
				uint32_t ptr_tid = cspv_ptr_type_id(ctx, chosen->params[i], CSpvStorageFunction);
				cspv_emit3(&ctx->local_vars, CSpvOpVariable, ptr_tid, temp, CSpvStorageFunction);
				if (qual == 2) { // inout: copy the current value in.
					cspv_value lv = vals[i];
					uint32_t cur = cspv_load(ctx, &lv);
					cspv_emit2(&ctx->body, CSpvOpStore, temp, cur);
				}
				cspv_copy_out co;
				co.temp = temp;
				co.arg = i;
				apush(copy_outs, co);
				w[n++] = temp;
			} else {
				cspv_value lv = vals[i];
				uint32_t id = cspv_load(ctx, &lv);
				w[n++] = cspv_convert(ctx, id, vals[i].type, chosen->params[i], e->line);
			}
		}
		cspv_emit(&ctx->body, CSpvOpFunctionCall, w, n);

		// Copy out/inout temps back into their argument lvalues.
		for (int i = 0; i < (int)asize(copy_outs); i++) {
			int arg = copy_outs[i].arg;
			uint32_t loaded = cspv_new_id(ctx);
			cspv_emit3(&ctx->body, CSpvOpLoad, cspv_type_id(ctx, chosen->params[arg]), loaded, copy_outs[i].temp);
			cspv_store(ctx, &vals[arg], loaded, e->line);
		}
		afree(copy_outs);
		afree(vals);
		return cspv_rvalue(chosen->type, result);
	}

	// Intrinsic?
	cspv_intrin* in = map_get_ptr(g_cspv_intrins, (uint64_t)(uintptr_t)name);
	if (in) return cspv_gen_intrin(ctx, e, in);

	cspv_errorf(ctx, e->line, "unknown function '%s'", name);
	cspv_value dummy;
	memset(&dummy, 0, sizeof(dummy));
	return dummy;
}

//--------------------------------------------------------------------------------------------------
// Full expression dispatch.

static cspv_value cspv_gen_expr(cspv_ctx* ctx, cspv_expr* e)
{
	switch (e->kind) {
	case CSPV_E_FLOAT_LIT:
		return cspv_rvalue(ctx->t_float, cspv_const_float(ctx, (float)e->u.fval));
	case CSPV_E_INT_LIT:
		return cspv_rvalue(ctx->t_int, cspv_const_int(ctx, (int32_t)e->u.ival));
	case CSPV_E_UINT_LIT:
		return cspv_rvalue(ctx->t_uint, cspv_const_uint(ctx, (uint32_t)e->u.ival));
	case CSPV_E_BOOL_LIT:
		return cspv_rvalue(ctx->t_bool, cspv_const_scalar(ctx, ctx->t_bool, e->u.bval ? 1 : 0));
	case CSPV_E_REF:
		return cspv_gen_ref(ctx, e);
	case CSPV_E_MEMBER:
		return cspv_gen_member(ctx, e);
	case CSPV_E_CALL:
		return cspv_gen_call(ctx, e);

	case CSPV_E_LENGTH: {
		// Runtime array in a buffer block: OpArrayLength on the block variable.
		if (e->u.member.base->kind == CSPV_E_REF) {
			cspv_symbol* sym = cspv_find_symbol(ctx, e->u.member.base->u.name);
			if (sym && sym->kind == CSPV_SYM_BLOCK_MEMBER &&
			    sym->type->kind == CSPV_T_ARRAY && sym->type->cols == -1) {
				uint32_t len = cspv_new_id(ctx);
				cspv_emit4(&ctx->body, CSpvOpArrayLength, cspv_type_id(ctx, ctx->t_uint), len, sym->id, (uint32_t)sym->member_index);
				uint32_t result = cspv_new_id(ctx);
				cspv_emit3(&ctx->body, CSpvOpBitcast, cspv_type_id(ctx, ctx->t_int), result, len);
				return cspv_rvalue(ctx->t_int, result);
			}
		}
		// Same, through a named block instance: `instance.member.length()`.
		if (e->u.member.base->kind == CSPV_E_MEMBER && e->u.member.base->u.member.base->kind == CSPV_E_REF) {
			cspv_symbol* sym = cspv_find_symbol(ctx, e->u.member.base->u.member.base->u.name);
			if (sym && sym->kind == CSPV_SYM_VAR && sym->type->kind == CSPV_T_STRUCT &&
			    (sym->storage == CSpvStorageStorageBuffer || sym->storage == CSpvStorageUniform)) {
				const char* field = e->u.member.base->u.member.member;
				for (int i = 0; i < (int)asize(sym->type->field_names); i++) {
					if (sym->type->field_names[i] == field &&
					    sym->type->field_types[i]->kind == CSPV_T_ARRAY && sym->type->field_types[i]->cols == -1) {
						uint32_t len = cspv_new_id(ctx);
						cspv_emit4(&ctx->body, CSpvOpArrayLength, cspv_type_id(ctx, ctx->t_uint), len, sym->id, (uint32_t)i);
						uint32_t result = cspv_new_id(ctx);
						cspv_emit3(&ctx->body, CSpvOpBitcast, cspv_type_id(ctx, ctx->t_int), result, len);
						return cspv_rvalue(ctx->t_int, result);
					}
				}
			}
		}
		cspv_value base = cspv_gen_expr(ctx, e->u.member.base);
		if (base.type->kind != CSPV_T_ARRAY || base.type->cols == -1) {
			cspv_errorf(ctx, e->line, ".length() requires an array");
		}
		return cspv_rvalue(ctx->t_int, cspv_const_int(ctx, base.type->cols));
	}

	case CSPV_E_INDEX: {
		cspv_value base = cspv_gen_expr(ctx, e->u.index.base);
		cspv_type* bt = base.type;
		if (bt->kind != CSPV_T_VEC && bt->kind != CSPV_T_MAT && bt->kind != CSPV_T_ARRAY) {
			cspv_errorf(ctx, e->line, "cannot index '%s'", cspv_type_name(bt));
		}
		cspv_type* it = NULL;
		uint32_t idx = cspv_gen_rvalue(ctx, e->u.index.index, &it);
		if (it != ctx->t_int && it != ctx->t_uint) cspv_errorf(ctx, e->line, "index must be an integer");
		cspv_type* elem = bt->kind == CSPV_T_VEC ? bt->elem :
		                  bt->kind == CSPV_T_ARRAY ? bt->elem : ctx->t_vec[bt->rows - 2];

		// Index through a pointer with OpAccessChain; spill rvalues (and swizzle
		// lvalues) into a Function temp first.
		cspv_value ptrv = base;
		if (base.kind != CSPV_V_PTR) ptrv = cspv_spill(ctx, &base);
		// Runtime arrays of structs in buffer blocks index into laid-out elements.
		uint32_t elem_laid = ptrv.layout_tid ? map_get(ctx->laid_elem_tids, ptrv.layout_tid) : 0;
		uint32_t ptr_tid = elem_laid ? cspv_ptr_type_id_raw(ctx, elem_laid, ptrv.storage)
		                             : cspv_ptr_type_id(ctx, elem, ptrv.storage);
		uint32_t ptr = cspv_new_id(ctx);
		cspv_emit4(&ctx->body, CSpvOpAccessChain, ptr_tid, ptr, ptrv.id, idx);
		cspv_value v;
		memset(&v, 0, sizeof(v));
		v.kind = CSPV_V_PTR;
		v.type = elem;
		v.id = ptr;
		v.storage = ptrv.storage;
		v.layout_tid = elem_laid;
		return v;
	}

	case CSPV_E_UNARY: {
		int op = e->u.un.op;
		if (op == CSPV_P_INC || op == CSPV_P_DEC) {
			cspv_value lv = cspv_gen_expr(ctx, e->u.un.e);
			if (lv.kind == CSPV_V_RVALUE) cspv_errorf(ctx, e->line, "operand of '%s' must be assignable", op == CSPV_P_INC ? "++" : "--");
			cspv_value loaded_lv = lv;
			uint32_t old_id = cspv_load(ctx, &loaded_lv);
			cspv_type* t = lv.type;
			cspv_type_kind ek = cspv_elem_type(t)->kind;
			uint32_t one;
			if (ek == CSPV_T_FLOAT) one = cspv_const_float(ctx, 1.0f);
			else if (ek == CSPV_T_UINT) one = cspv_const_uint(ctx, 1);
			else one = cspv_const_int(ctx, 1);
			if (cspv_is_vector(t)) {
				cspv_type* dummy = NULL;
				one = cspv_splat(ctx, one, cspv_elem_type(t), t->cols, &dummy);
			}
			uint32_t result = cspv_new_id(ctx);
			int opcode = (ek == CSPV_T_FLOAT) ? (op == CSPV_P_INC ? CSpvOpFAdd : CSpvOpFSub)
			                                  : (op == CSPV_P_INC ? CSpvOpIAdd : CSpvOpISub);
			cspv_emit4(&ctx->body, opcode, cspv_type_id(ctx, t), result, old_id, one);
			cspv_store(ctx, &lv, result, e->line);
			return cspv_rvalue(t, e->u.un.postfix ? old_id : result);
		}

		cspv_type* t = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.un.e, &t);
		cspv_type_kind ek = cspv_elem_type(t)->kind;
		uint32_t result = cspv_new_id(ctx);
		if (op == '-') {
			if (t->kind == CSPV_T_MAT) {
				// OpFNegate only takes scalars/vectors; negate via m * -1.0.
				uint32_t neg_one = cspv_const_float(ctx, -1.0f);
				cspv_emit4(&ctx->body, CSpvOpMatrixTimesScalar, cspv_type_id(ctx, t), result, a, neg_one);
			}
			else if (ek == CSPV_T_FLOAT) cspv_emit3(&ctx->body, CSpvOpFNegate, cspv_type_id(ctx, t), result, a);
			else if (ek == CSPV_T_INT || ek == CSPV_T_UINT) cspv_emit3(&ctx->body, CSpvOpSNegate, cspv_type_id(ctx, t), result, a);
			else cspv_errorf(ctx, e->line, "cannot negate '%s'", cspv_type_name(t));
		} else if (op == '!') {
			if (t != ctx->t_bool) cspv_errorf(ctx, e->line, "'!' requires a bool operand");
			cspv_emit3(&ctx->body, CSpvOpLogicalNot, cspv_type_id(ctx, t), result, a);
		} else if (op == '~') {
			if (ek != CSPV_T_INT && ek != CSPV_T_UINT) cspv_errorf(ctx, e->line, "'~' requires integer operands");
			cspv_emit3(&ctx->body, CSpvOpNot, cspv_type_id(ctx, t), result, a);
		} else {
			cspv_errorf(ctx, e->line, "unsupported unary operator");
		}
		return cspv_rvalue(t, result);
	}

	case CSPV_E_BINARY: {
		int op = e->u.bin.op;

		if (op == ',') {
			// Sequence: evaluate left for effects, result is the right side.
			cspv_gen_expr(ctx, e->u.bin.l);
			return cspv_gen_expr(ctx, e->u.bin.r);
		}

		if (op == '=') {
			cspv_value lv = cspv_gen_expr(ctx, e->u.bin.l);
			if (lv.kind == CSPV_V_RVALUE) cspv_errorf(ctx, e->line, "left side of '=' is not assignable");
			uint32_t src = cspv_gen_rvalue_as(ctx, e->u.bin.r, lv.type);
			cspv_store(ctx, &lv, src, e->line);
			return cspv_rvalue(lv.type, src);
		}

		int base = cspv_compound_base_op(op);
		if (base) {
			cspv_value lv = cspv_gen_expr(ctx, e->u.bin.l);
			if (lv.kind == CSPV_V_RVALUE) cspv_errorf(ctx, e->line, "left side of assignment is not assignable");
			cspv_value loaded_lv = lv;
			uint32_t a = cspv_load(ctx, &loaded_lv);
			cspv_type* bt = NULL;
			uint32_t b = cspv_gen_rvalue(ctx, e->u.bin.r, &bt);
			cspv_value r = cspv_gen_binop(ctx, base, e->line, a, lv.type, b, bt);
			if (r.type != lv.type && !cspv_implicitly_converts(r.type, lv.type)) {
				cspv_errorf(ctx, e->line, "cannot assign '%s' to '%s'", cspv_type_name(r.type), cspv_type_name(lv.type));
			}
			uint32_t conv = cspv_convert(ctx, r.id, r.type, lv.type, e->line);
			cspv_store(ctx, &lv, conv, e->line);
			return cspv_rvalue(lv.type, conv);
		}

		if (op == CSPV_P_AND || op == CSPV_P_OR) {
			// Short-circuit: `a && b` is `tmp = a; if (a) tmp = b;` and `a || b`
			// is `tmp = a; if (!a) tmp = b;`.
			cspv_type* at = NULL;
			uint32_t a = cspv_gen_rvalue(ctx, e->u.bin.l, &at);
			if (at != ctx->t_bool) cspv_errorf(ctx, e->line, "'&&'/'||' require bool operands");

			uint32_t tmp = cspv_new_id(ctx);
			uint32_t bool_ptr_tid = cspv_ptr_type_id(ctx, ctx->t_bool, CSpvStorageFunction);
			cspv_emit3(&ctx->local_vars, CSpvOpVariable, bool_ptr_tid, tmp, CSpvStorageFunction);
			cspv_emit2(&ctx->body, CSpvOpStore, tmp, a);

			uint32_t rhs_label = cspv_new_id(ctx);
			uint32_t merge = cspv_new_id(ctx);
			cspv_emit2(&ctx->body, CSpvOpSelectionMerge, merge, 0);
			if (op == CSPV_P_AND) cspv_emit3(&ctx->body, CSpvOpBranchConditional, a, rhs_label, merge);
			else cspv_emit3(&ctx->body, CSpvOpBranchConditional, a, merge, rhs_label);
			ctx->block_terminated = true;

			cspv_begin_block(ctx, rhs_label);
			cspv_type* bt = NULL;
			uint32_t b = cspv_gen_rvalue(ctx, e->u.bin.r, &bt);
			if (bt != ctx->t_bool) cspv_errorf(ctx, e->line, "'&&'/'||' require bool operands");
			cspv_emit2(&ctx->body, CSpvOpStore, tmp, b);
			cspv_branch(ctx, merge);

			cspv_begin_block(ctx, merge);
			uint32_t result = cspv_new_id(ctx);
			cspv_emit3(&ctx->body, CSpvOpLoad, cspv_type_id(ctx, ctx->t_bool), result, tmp);
			return cspv_rvalue(ctx->t_bool, result);
		}

		cspv_type* at = NULL;
		cspv_type* bt = NULL;
		uint32_t a = cspv_gen_rvalue(ctx, e->u.bin.l, &at);
		uint32_t b = cspv_gen_rvalue(ctx, e->u.bin.r, &bt);
		return cspv_gen_binop(ctx, op, e->line, a, at, b, bt);
	}

	case CSPV_E_COND: {
		// Branch-based ternary: only the taken arm evaluates (GLSL semantics).
		// The result type comes from a discarded dry run of both arms.
		cspv_type* ct = NULL;
		uint32_t c = cspv_gen_rvalue(ctx, e->u.cond.c, &ct);
		if (ct != ctx->t_bool) cspv_errorf(ctx, e->line, "ternary condition must be bool");

		cspv_type* at = cspv_dry_run_type(ctx, e->u.cond.a);
		cspv_type* bt = cspv_dry_run_type(ctx, e->u.cond.b);
		cspv_type* rt = cspv_unify_types(ctx, at, bt, e->line);

		uint32_t tmp = cspv_new_id(ctx);
		uint32_t ptr_tid = cspv_ptr_type_id(ctx, rt, CSpvStorageFunction);
		cspv_emit3(&ctx->local_vars, CSpvOpVariable, ptr_tid, tmp, CSpvStorageFunction);

		uint32_t then_label = cspv_new_id(ctx);
		uint32_t else_label = cspv_new_id(ctx);
		uint32_t merge = cspv_new_id(ctx);
		cspv_emit2(&ctx->body, CSpvOpSelectionMerge, merge, 0);
		cspv_emit3(&ctx->body, CSpvOpBranchConditional, c, then_label, else_label);
		ctx->block_terminated = true;

		cspv_begin_block(ctx, then_label);
		cspv_emit2(&ctx->body, CSpvOpStore, tmp, cspv_gen_rvalue_as(ctx, e->u.cond.a, rt));
		cspv_branch(ctx, merge);

		cspv_begin_block(ctx, else_label);
		cspv_emit2(&ctx->body, CSpvOpStore, tmp, cspv_gen_rvalue_as(ctx, e->u.cond.b, rt));
		cspv_branch(ctx, merge);

		cspv_begin_block(ctx, merge);
		uint32_t result = cspv_new_id(ctx);
		cspv_emit3(&ctx->body, CSpvOpLoad, cspv_type_id(ctx, rt), result, tmp);
		return cspv_rvalue(rt, result);
	}
	}

	cspv_errorf(ctx, e->line, "internal: unhandled expression");
	cspv_value dummy;
	memset(&dummy, 0, sizeof(dummy));
	return dummy;
}

//--------------------------------------------------------------------------------------------------
// Statement codegen.

static cspv_type* cspv_make_type(cspv_ctx* ctx, cspv_type_kind kind, cspv_type* elem, int cols, int rows);

// Map a cspv type to the reflection data-type enum (CF_ShaderInfoDataType values).
static CSPV_DataType cspv_reflect_type(cspv_type* t)
{
	switch (t->kind) {
	case CSPV_T_FLOAT: return CSPV_TYPE_FLOAT;
	case CSPV_T_INT:   return CSPV_TYPE_SINT;
	case CSPV_T_UINT:  return CSPV_TYPE_UINT;
	case CSPV_T_MAT:   return t->cols == 4 ? CSPV_TYPE_MAT4 : CSPV_TYPE_UNKNOWN;
	case CSPV_T_VEC:
		switch (t->elem->kind) {
		case CSPV_T_FLOAT: return (CSPV_DataType)(CSPV_TYPE_FLOAT2 + (t->cols - 2) * 3);
		case CSPV_T_INT:   return (CSPV_DataType)(CSPV_TYPE_SINT2 + (t->cols - 2) * 3);
		case CSPV_T_UINT:  return (CSPV_DataType)(CSPV_TYPE_UINT2 + (t->cols - 2) * 3);
		default: return CSPV_TYPE_UNKNOWN;
		}
	default: return CSPV_TYPE_UNKNOWN;
	}
}

static void cspv_begin_block(cspv_ctx* ctx, uint32_t label_id)
{
	cspv_emit1(&ctx->body, CSpvOpLabel, label_id);
	ctx->block_terminated = false;
}

static void cspv_branch(cspv_ctx* ctx, uint32_t target)
{
	cspv_emit1(&ctx->body, CSpvOpBranch, target);
	ctx->block_terminated = true;
}

static void cspv_gen_stmt(cspv_ctx* ctx, cspv_stmt* s)
{
	// Code after a terminator (e.g. statements after `return`) goes into a fresh
	// unreachable block, same as glslang.
	if (ctx->block_terminated) {
		cspv_begin_block(ctx, cspv_new_id(ctx));
	}

	switch (s->kind) {
	case CSPV_S_BLOCK: {
		cspv_push_scope(ctx);
		for (int i = 0; i < (int)asize(s->u.block); i++) {
			cspv_gen_stmt(ctx, s->u.block[i]);
		}
		cspv_pop_scope(ctx);
		break;
	}

	case CSPV_S_DECL: {
		cspv_type* type = s->u.decl.type;
		uint32_t var = cspv_new_id(ctx);
		uint32_t ptr_tid = cspv_ptr_type_id(ctx, type, CSpvStorageFunction);
		cspv_emit3(&ctx->local_vars, CSpvOpVariable, ptr_tid, var, CSpvStorageFunction);
		cspv_name_id(ctx, var, s->u.decl.name);
		cspv_add_symbol(ctx, s->u.decl.name, CSPV_SYM_VAR, type, var, CSpvStorageFunction, s->line);
		if (s->u.decl.init) {
			uint32_t init = cspv_gen_rvalue_as(ctx, s->u.decl.init, type);
			cspv_emit2(&ctx->body, CSpvOpStore, var, init);
		}
		if (s->u.decl.next_decl) cspv_gen_stmt(ctx, s->u.decl.next_decl);
		break;
	}

	case CSPV_S_EXPR:
		cspv_gen_expr(ctx, s->u.expr);
		break;

	case CSPV_S_IF: {
		cspv_type* ct = NULL;
		uint32_t cond = cspv_gen_rvalue(ctx, s->u.if_s.cond, &ct);
		if (ct != ctx->t_bool) cspv_errorf(ctx, s->line, "if condition must be bool");
		uint32_t merge = cspv_new_id(ctx);
		uint32_t then_label = cspv_new_id(ctx);
		uint32_t else_label = s->u.if_s.else_s ? cspv_new_id(ctx) : merge;
		cspv_emit2(&ctx->body, CSpvOpSelectionMerge, merge, 0);
		cspv_emit3(&ctx->body, CSpvOpBranchConditional, cond, then_label, else_label);
		ctx->block_terminated = true;

		cspv_begin_block(ctx, then_label);
		cspv_push_scope(ctx);
		cspv_gen_stmt(ctx, s->u.if_s.then_s);
		cspv_pop_scope(ctx);
		if (!ctx->block_terminated) cspv_branch(ctx, merge);

		if (s->u.if_s.else_s) {
			cspv_begin_block(ctx, else_label);
			cspv_push_scope(ctx);
			cspv_gen_stmt(ctx, s->u.if_s.else_s);
			cspv_pop_scope(ctx);
			if (!ctx->block_terminated) cspv_branch(ctx, merge);
		}

		cspv_begin_block(ctx, merge);
		break;
	}

	case CSPV_S_FOR:
	case CSPV_S_WHILE: {
		bool is_for = s->kind == CSPV_S_FOR;
		cspv_push_scope(ctx); // Loop-header declarations live in their own scope.
		if (is_for && s->u.for_s.init) cspv_gen_stmt(ctx, s->u.for_s.init);

		uint32_t header = cspv_new_id(ctx);
		uint32_t cond_label = cspv_new_id(ctx);
		uint32_t body_label = cspv_new_id(ctx);
		uint32_t continue_label = cspv_new_id(ctx);
		uint32_t merge = cspv_new_id(ctx);

		cspv_branch(ctx, header);
		cspv_begin_block(ctx, header);
		cspv_emit3(&ctx->body, CSpvOpLoopMerge, merge, continue_label, 0);
		cspv_branch(ctx, cond_label);

		cspv_begin_block(ctx, cond_label);
		cspv_expr* cond_expr = is_for ? s->u.for_s.cond : s->u.while_s.cond;
		uint32_t cond;
		if (cond_expr) {
			cspv_type* ct = NULL;
			cond = cspv_gen_rvalue(ctx, cond_expr, &ct);
			if (ct != ctx->t_bool) cspv_errorf(ctx, s->line, "loop condition must be bool");
		} else {
			cond = cspv_const_scalar(ctx, ctx->t_bool, 1);
		}
		cspv_emit3(&ctx->body, CSpvOpBranchConditional, cond, body_label, merge);
		ctx->block_terminated = true;

		uint32_t saved_break = ctx->break_target;
		uint32_t saved_continue = ctx->continue_target;
		ctx->break_target = merge;
		ctx->continue_target = continue_label;

		cspv_begin_block(ctx, body_label);
		cspv_push_scope(ctx);
		cspv_gen_stmt(ctx, is_for ? s->u.for_s.body : s->u.while_s.body);
		cspv_pop_scope(ctx);
		if (!ctx->block_terminated) cspv_branch(ctx, continue_label);

		ctx->break_target = saved_break;
		ctx->continue_target = saved_continue;

		cspv_begin_block(ctx, continue_label);
		if (is_for && s->u.for_s.iter) cspv_gen_expr(ctx, s->u.for_s.iter);
		cspv_branch(ctx, header);

		cspv_begin_block(ctx, merge);
		cspv_pop_scope(ctx);
		break;
	}

	case CSPV_S_DO: {
		cspv_push_scope(ctx);
		uint32_t header = cspv_new_id(ctx);
		uint32_t body_label = cspv_new_id(ctx);
		uint32_t continue_label = cspv_new_id(ctx);
		uint32_t merge = cspv_new_id(ctx);

		cspv_branch(ctx, header);
		cspv_begin_block(ctx, header);
		cspv_emit3(&ctx->body, CSpvOpLoopMerge, merge, continue_label, 0);
		cspv_branch(ctx, body_label);

		uint32_t saved_break = ctx->break_target;
		uint32_t saved_continue = ctx->continue_target;
		ctx->break_target = merge;
		ctx->continue_target = continue_label;

		cspv_begin_block(ctx, body_label);
		cspv_gen_stmt(ctx, s->u.while_s.body);
		if (!ctx->block_terminated) cspv_branch(ctx, continue_label);

		ctx->break_target = saved_break;
		ctx->continue_target = saved_continue;

		// The condition lives in the continue block; the back edge is conditional.
		cspv_begin_block(ctx, continue_label);
		cspv_type* ct = NULL;
		uint32_t cond = cspv_gen_rvalue(ctx, s->u.while_s.cond, &ct);
		if (ct != ctx->t_bool) cspv_errorf(ctx, s->line, "loop condition must be bool");
		cspv_emit3(&ctx->body, CSpvOpBranchConditional, cond, header, merge);
		ctx->block_terminated = true;

		cspv_begin_block(ctx, merge);
		cspv_pop_scope(ctx);
		break;
	}

	case CSPV_S_SWITCH: {
		cspv_type* st = NULL;
		uint32_t sel = cspv_gen_rvalue(ctx, s->u.switch_s.sel, &st);
		if (st != ctx->t_int && st != ctx->t_uint) cspv_errorf(ctx, s->line, "switch selector must be int or uint");

		int num_groups = (int)asize(s->u.switch_s.groups);
		uint32_t merge = cspv_new_id(ctx);
		CK_DYNA uint32_t* group_labels = NULL;
		uint32_t default_label = merge;
		for (int i = 0; i < num_groups; i++) {
			uint32_t label = cspv_new_id(ctx);
			apush(group_labels, label);
			if (s->u.switch_s.groups[i].is_default) default_label = label;
		}

		cspv_emit2(&ctx->body, CSpvOpSelectionMerge, merge, 0);
		CK_DYNA uint32_t* w = NULL;
		apush(w, sel);
		apush(w, default_label);
		for (int i = 0; i < num_groups; i++) {
			cspv_switch_group* g = &s->u.switch_s.groups[i];
			for (int j = 0; j < (int)asize(g->labels); j++) {
				apush(w, (uint32_t)(int32_t)g->labels[j]);
				apush(w, group_labels[i]);
			}
		}
		cspv_emit(&ctx->body, CSpvOpSwitch, w, (int)asize(w));
		afree(w);
		ctx->block_terminated = true;

		uint32_t saved_break = ctx->break_target;
		ctx->break_target = merge; // `continue` still targets the enclosing loop.

		for (int i = 0; i < num_groups; i++) {
			cspv_switch_group* g = &s->u.switch_s.groups[i];
			cspv_begin_block(ctx, group_labels[i]);
			cspv_push_scope(ctx);
			for (int j = 0; j < (int)asize(g->stmts); j++) {
				cspv_gen_stmt(ctx, g->stmts[j]);
			}
			cspv_pop_scope(ctx);
			if (!ctx->block_terminated) {
				// Fallthrough to the next group, or exit the switch after the last.
				cspv_branch(ctx, i + 1 < num_groups ? group_labels[i + 1] : merge);
			}
		}

		ctx->break_target = saved_break;
		afree(group_labels);
		cspv_begin_block(ctx, merge);
		break;
	}

	case CSPV_S_RETURN: {
		ctx->func_has_return = true;
		cspv_type* ret_type = ctx->cur_func ? ctx->cur_func->type : ctx->t_void;
		if (s->u.ret) {
			if (ret_type == ctx->t_void) cspv_errorf(ctx, s->line, "void function cannot return a value");
			uint32_t v = cspv_gen_rvalue_as(ctx, s->u.ret, ret_type);
			cspv_emit1(&ctx->body, CSpvOpReturnValue, v);
		} else {
			if (ret_type != ctx->t_void) cspv_errorf(ctx, s->line, "non-void function must return a value");
			cspv_emit(&ctx->body, CSpvOpReturn, NULL, 0);
		}
		ctx->block_terminated = true;
		break;
	}

	case CSPV_S_DISCARD:
		if (ctx->stage != CSPV_STAGE_FRAGMENT) cspv_errorf(ctx, s->line, "discard is only valid in fragment shaders");
		cspv_emit(&ctx->body, CSpvOpKill, NULL, 0);
		ctx->block_terminated = true;
		break;

	case CSPV_S_BREAK:
		if (!ctx->break_target) cspv_errorf(ctx, s->line, "break outside of a loop");
		cspv_branch(ctx, ctx->break_target);
		break;

	case CSPV_S_CONTINUE:
		if (!ctx->continue_target) cspv_errorf(ctx, s->line, "continue outside of a loop");
		cspv_branch(ctx, ctx->continue_target);
		break;
	}
}

//--------------------------------------------------------------------------------------------------
// Function definitions.

typedef struct cspv_param
{
	cspv_type* type;
	const char* name;
	int qual; // 0 = in, 1 = out, 2 = inout.
} cspv_param;

static uint32_t cspv_fn_type_id(cspv_ctx* ctx, cspv_type* ret, CK_DYNA cspv_param* params)
{
	// OpTypeFunction must also be unique. Key on an FNV-1a hash of the type ids.
	// out/inout parameters are Function-storage pointers.
	uint32_t ret_tid = cspv_type_id(ctx, ret);
	CK_DYNA uint32_t* param_tids = NULL;
	for (int i = 0; i < (int)asize(params); i++) {
		apush(param_tids, params[i].qual ? cspv_ptr_type_id(ctx, params[i].type, CSpvStorageFunction)
		                                 : cspv_type_id(ctx, params[i].type));
	}
	uint64_t h = 14695981039346656037ull;
	h = (h ^ ret_tid) * 1099511628211ull;
	for (int i = 0; i < (int)asize(param_tids); i++) {
		h = (h ^ param_tids[i]) * 1099511628211ull;
	}
	uint32_t* found = map_get_ptr(ctx->fn_type_ids, h);
	if (found) {
		afree(param_tids);
		return *found;
	}
	uint32_t id = cspv_new_id(ctx);
	CK_DYNA uint32_t* w = NULL;
	apush(w, id);
	apush(w, ret_tid);
	for (int i = 0; i < (int)asize(param_tids); i++) apush(w, param_tids[i]);
	cspv_emit(&ctx->globals, CSpvOpTypeFunction, w, (int)asize(w));
	afree(w);
	afree(param_tids);
	map_set(ctx->fn_type_ids, h, id);
	return id;
}

static void cspv_gen_function(cspv_ctx* ctx, cspv_type* ret, const char* name, CK_DYNA cspv_param* params, cspv_stmt* body, int line)
{
	// Collect param types and qualifiers.
	CK_DYNA cspv_type** param_types = NULL;
	CK_DYNA int* param_quals = NULL;
	for (int i = 0; i < (int)asize(params); i++) {
		apush(param_types, params[i].type);
		apush(param_quals, params[i].qual);
	}

	// Reject redefinition with an identical signature.
	cspv_symbol* head = map_get(ctx->functions, (uint64_t)(uintptr_t)name);
	for (cspv_symbol* o = head; o; o = o->next_overload) {
		if ((int)asize(o->params) != (int)asize(param_types)) continue;
		bool same = true;
		for (int i = 0; i < (int)asize(param_types); i++) {
			if (o->params[i] != param_types[i]) { same = false; break; }
		}
		if (same) cspv_errorf(ctx, line, "redefinition of '%s'", name);
	}

	uint32_t fid = cspv_new_id(ctx);
	uint32_t fn_tid = cspv_fn_type_id(ctx, ret, params);
	uint32_t ret_tid = cspv_type_id(ctx, ret);
	cspv_name_id(ctx, fid, name);

	// Allocate parameter value ids.
	CK_DYNA uint32_t* param_ids = NULL;
	for (int i = 0; i < (int)asize(params); i++) {
		apush(param_ids, cspv_new_id(ctx));
	}

	// Generate the body into fresh buffers.
	CK_DYNA uint32_t* saved_body = ctx->body;
	CK_DYNA uint32_t* saved_locals = ctx->local_vars;
	ctx->body = NULL;
	ctx->local_vars = NULL;
	ctx->block_terminated = false;

	cspv_symbol fn_sym_tmp;
	memset(&fn_sym_tmp, 0, sizeof(fn_sym_tmp));
	fn_sym_tmp.type = ret;
	cspv_symbol* saved_func = ctx->cur_func;
	ctx->cur_func = &fn_sym_tmp;
	bool saved_has_return = ctx->func_has_return;
	ctx->func_has_return = false;

	cspv_push_scope(ctx);
	// `in` parameters are mutable in GLSL: copy each into a Function-storage
	// variable. `out`/`inout` parameters are already Function-storage pointers.
	for (int i = 0; i < (int)asize(params); i++) {
		if (params[i].qual) {
			cspv_name_id(ctx, param_ids[i], params[i].name);
			cspv_add_symbol(ctx, params[i].name, CSPV_SYM_VAR, params[i].type, param_ids[i], CSpvStorageFunction, line);
			continue;
		}
		uint32_t var = cspv_new_id(ctx);
		uint32_t ptr_tid = cspv_ptr_type_id(ctx, params[i].type, CSpvStorageFunction);
		cspv_emit3(&ctx->local_vars, CSpvOpVariable, ptr_tid, var, CSpvStorageFunction);
		cspv_emit2(&ctx->body, CSpvOpStore, var, param_ids[i]);
		cspv_name_id(ctx, var, params[i].name);
		cspv_add_symbol(ctx, params[i].name, CSPV_SYM_VAR, params[i].type, var, CSpvStorageFunction, line);
	}
	cspv_gen_stmt(ctx, body);
	cspv_pop_scope(ctx);

	if (!ctx->block_terminated) {
		if (ret == ctx->t_void) {
			cspv_emit(&ctx->body, CSpvOpReturn, NULL, 0);
		} else if (ctx->func_has_return) {
			// e.g. a function ending in an if/else where both branches return: the
			// merge block exists but is unreachable.
			cspv_emit(&ctx->body, CSpvOpUnreachable, NULL, 0);
		} else {
			cspv_errorf(ctx, line, "function '%s' is missing a return statement", name);
		}
	}

	// Splice the function together.
	uint32_t fn_header[4] = { ret_tid, fid, 0 /* FunctionControlNone */, fn_tid };
	cspv_emit(&ctx->funcs, CSpvOpFunction, fn_header, 4);
	for (int i = 0; i < (int)asize(params); i++) {
		uint32_t param_tid = params[i].qual ? cspv_ptr_type_id(ctx, params[i].type, CSpvStorageFunction)
		                                    : cspv_type_id(ctx, params[i].type);
		cspv_emit2(&ctx->funcs, CSpvOpFunctionParameter, param_tid, param_ids[i]);
	}
	cspv_emit1(&ctx->funcs, CSpvOpLabel, cspv_new_id(ctx));
	for (int i = 0; i < (int)asize(ctx->local_vars); i++) apush(ctx->funcs, ctx->local_vars[i]);
	for (int i = 0; i < (int)asize(ctx->body); i++) apush(ctx->funcs, ctx->body[i]);
	cspv_emit(&ctx->funcs, CSpvOpFunctionEnd, NULL, 0);

	afree(ctx->body);
	afree(ctx->local_vars);
	ctx->body = saved_body;
	ctx->local_vars = saved_locals;
	ctx->cur_func = saved_func;
	ctx->func_has_return = saved_has_return;

	// Register (after body gen, so recursion is naturally rejected as undeclared).
	cspv_symbol* sym = (cspv_symbol*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_symbol));
	memset(sym, 0, sizeof(*sym));
	sym->kind = CSPV_SYM_FUNC;
	sym->name = name;
	sym->type = ret;
	sym->id = fid;
	sym->params = param_types;
	sym->param_quals = param_quals;
	sym->next_overload = head;
	map_set(ctx->functions, (uint64_t)(uintptr_t)name, sym);

	if (name == g_cspv_kw.kw_main) {
		if (ret != ctx->t_void || asize(params) != 0) cspv_errorf(ctx, line, "main must be 'void main()'");
		ctx->entry_func_id = fid;
	}
	afree(param_ids);
}

//--------------------------------------------------------------------------------------------------
// Global declarations.

// std140 layout rules for the supported uniform block member types.
static void cspv_std140_layout(cspv_ctx* ctx, cspv_type* t, int line, int* align, int* size)
{
	switch (t->kind) {
	case CSPV_T_FLOAT: case CSPV_T_INT: case CSPV_T_UINT:
		*align = 4; *size = 4;
		return;
	case CSPV_T_VEC:
		if (t->cols == 2) { *align = 8; *size = 8; }
		else if (t->cols == 3) { *align = 16; *size = 12; }
		else { *align = 16; *size = 16; }
		return;
	case CSPV_T_MAT:
		// Column-major with vec4-aligned columns (std140).
		*align = 16;
		*size = t->cols * 16;
		return;
	default:
		cspv_errorf(ctx, line, "type '%s' is not supported in uniform blocks yet", cspv_type_name(t));
	}
}

// std430 layout for buffer-block member types (scalars, vectors, and structs of
// those). Struct alignment is the max member alignment; size rounds up to it.
static void cspv_std430_layout(cspv_ctx* ctx, cspv_type* t, int line, int* align, int* size)
{
	switch (t->kind) {
	case CSPV_T_FLOAT: case CSPV_T_INT: case CSPV_T_UINT:
		*align = 4; *size = 4;
		return;
	case CSPV_T_VEC:
		if (t->cols == 2) { *align = 8; *size = 8; }
		else if (t->cols == 3) { *align = 16; *size = 12; }
		else { *align = 16; *size = 16; }
		return;
	case CSPV_T_STRUCT: {
		int max_align = 4;
		int offset = 0;
		for (int i = 0; i < (int)asize(t->field_types); i++) {
			int fa = 0, fs = 0;
			cspv_std430_layout(ctx, t->field_types[i], line, &fa, &fs);
			offset = (offset + fa - 1) / fa * fa + fs;
			if (fa > max_align) max_align = fa;
		}
		*align = max_align;
		*size = (offset + max_align - 1) / max_align * max_align;
		return;
	}
	default:
		cspv_errorf(ctx, line, "type '%s' is not supported in buffer blocks", cspv_type_name(t));
	}
}

// Laid-out spirv struct type for buffer-block interiors: same fields as the plain
// struct, plus std430 Offset decorations. Distinct from the plain type because
// offset-decorated types may not be used in Function storage.
static uint32_t cspv_laid_struct_tid(cspv_ctx* ctx, cspv_type* t, int line)
{
	uint32_t* found = map_get_ptr(ctx->laid_struct_tids, (uint64_t)(uintptr_t)t);
	if (found) return *found;

	CK_DYNA uint32_t* w = NULL;
	uint32_t id = cspv_new_id(ctx);
	apush(w, id);
	for (int i = 0; i < (int)asize(t->field_types); i++) {
		cspv_type* ft = t->field_types[i];
		if (!cspv_is_scalar(ft) && !cspv_is_vector(ft)) {
			cspv_errorf(ctx, line, "buffer-block struct fields must be scalars or vectors ('%s.%s')", t->name, t->field_names[i]);
		}
		apush(w, cspv_type_id(ctx, ft));
	}
	cspv_emit(&ctx->globals, CSpvOpTypeStruct, w, (int)asize(w));
	afree(w);

	int offset = 0;
	for (int i = 0; i < (int)asize(t->field_types); i++) {
		int fa = 0, fs = 0;
		cspv_std430_layout(ctx, t->field_types[i], line, &fa, &fs);
		offset = (offset + fa - 1) / fa * fa;
		uint32_t deco[4] = { id, (uint32_t)i, CSpvDecorationOffset, (uint32_t)offset };
		cspv_emit(&ctx->decos, CSpvOpMemberDecorate, deco, 4);
		uint32_t mn[2] = { id, (uint32_t)i };
		cspv_emit_str(&ctx->names, CSpvOpMemberName, mn, 2, t->field_names[i]);
		offset += fs;
	}
	cspv_name_id(ctx, id, t->name);

	map_set(ctx->laid_struct_tids, (uint64_t)(uintptr_t)t, id);
	return id;
}

// Runtime array of a laid-out struct (SSBO tails), with its std430 ArrayStride.
static uint32_t cspv_laid_runtime_array_tid(cspv_ctx* ctx, cspv_type* elem, int line)
{
	uint32_t* found = map_get_ptr(ctx->laid_array_tids, (uint64_t)(uintptr_t)elem);
	if (found) return *found;
	uint32_t elem_tid = cspv_laid_struct_tid(ctx, elem, line);
	int align = 0, stride = 0;
	cspv_std430_layout(ctx, elem, line, &align, &stride);
	uint32_t id = cspv_new_id(ctx);
	cspv_emit2(&ctx->globals, CSpvOpTypeRuntimeArray, id, elem_tid);
	cspv_emit3(&ctx->decos, CSpvOpDecorate, id, CSpvDecorationArrayStride, (uint32_t)stride);
	map_set(ctx->laid_array_tids, (uint64_t)(uintptr_t)elem, id);
	map_set(ctx->laid_elem_tids, id, elem_tid);
	return id;
}

static void cspv_gen_global_inout(cspv_ctx* ctx, cspv_layout* layout, bool is_input, bool flat, cspv_type* type, const char* name, int line)
{
	if (layout->location < 0) {
		cspv_errorf(ctx, line, "'%s' requires an explicit layout(location = N)", name);
	}
	int storage = is_input ? CSpvStorageInput : CSpvStorageOutput;
	uint32_t var = cspv_new_id(ctx);
	uint32_t ptr_tid = cspv_ptr_type_id(ctx, type, storage);
	cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_tid, var, (uint32_t)storage);
	cspv_emit3(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationLocation, (uint32_t)layout->location);
	// Integer varyings must interpolate flat. Decorate them implicitly on both the
	// vertex-output and fragment-input sides (vertex *inputs* are attributes and
	// must not be decorated) so downstream consumers -- Vulkan validation and the
	// GLSL ES output -- see correct decorations without any runtime fixups.
	bool integer_varying = cspv_elem_type(type)->kind != CSPV_T_FLOAT &&
		((is_input && ctx->stage == CSPV_STAGE_FRAGMENT) ||
		 (!is_input && ctx->stage == CSPV_STAGE_VERTEX));
	if (flat || integer_varying) {
		cspv_emit2(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationFlat);
	}
	cspv_name_id(ctx, var, name);
	cspv_add_symbol(ctx, name, CSPV_SYM_VAR, type, var, storage, line);
	apush(ctx->interface_ids, var);

	if (is_input && ctx->stage == CSPV_STAGE_VERTEX) {
		CSPV_ReflectionInput ri;
		ri.name = name;
		ri.location = layout->location;
		ri.type = cspv_reflect_type(type);
		apush(ctx->reflection.inputs, ri);
	}
}

// Shared generator for uniform (std140, Uniform storage) and buffer (std430,
// StorageBuffer storage) blocks. Buffer blocks may end with a runtime array member.
static void cspv_gen_uniform_block(cspv_ctx* ctx, cspv_layout* layout, const char* block_name, bool is_buffer, bool readonly, int line)
{
	if (layout->set < 0 || layout->binding < 0) {
		cspv_errorf(ctx, line, "%s block '%s' requires layout(set = N, binding = M)", is_buffer ? "buffer" : "uniform", block_name);
	}

	CK_DYNA cspv_type** member_types = NULL;
	CK_DYNA const char** member_names = NULL;
	CK_DYNA int* member_lines = NULL;

	cspv_expect_punct(ctx, '{');
	while (!cspv_is_punct(ctx, '}')) {
		int mline = ctx->tok.line;
		cspv_type* mtype = cspv_parse_type(ctx);
		const char* mname = cspv_expect_any_ident(ctx);
		// `type name[];` declares a runtime array (buffer blocks only, last member).
		if (cspv_accept_punct(ctx, '[')) {
			if (cspv_accept_punct(ctx, ']')) {
				if (!is_buffer) cspv_errorf(ctx, mline, "runtime arrays are only allowed in buffer blocks");
				if (mtype->kind != CSPV_T_FLOAT && mtype->kind != CSPV_T_INT && mtype->kind != CSPV_T_UINT &&
				    mtype->kind != CSPV_T_VEC && mtype->kind != CSPV_T_STRUCT) {
					cspv_errorf(ctx, mline, "runtime array elements must be scalars, vectors, or structs");
				}
				mtype = cspv_array_type(ctx, mtype, -1);
			} else {
				cspv_errorf(ctx, mline, "sized arrays in blocks are not supported yet");
			}
		}
		if (mtype->kind == CSPV_T_STRUCT && !is_buffer) {
			cspv_errorf(ctx, mline, "struct members are only supported in buffer blocks");
		}
		cspv_expect_punct(ctx, ';');
		if (asize(member_types) && alast(member_types)->kind == CSPV_T_ARRAY && alast(member_types)->cols == -1) {
			cspv_errorf(ctx, mline, "a runtime array must be the last member of its block");
		}
		apush(member_types, mtype);
		apush(member_names, mname);
		apush(member_lines, mline);
	}
	cspv_advance(ctx); // '}'
	// Optional instance name: members are then accessed as `instance.member`
	// instead of as bare names.
	const char* instance_name = NULL;
	if (ctx->tok.kind == CSPV_TOK_IDENT) {
		instance_name = ctx->tok.ident;
		cspv_advance(ctx);
	}
	cspv_expect_punct(ctx, ';');

	if (asize(member_types) == 0) cspv_errorf(ctx, line, "%s block '%s' is empty", is_buffer ? "buffer" : "uniform", block_name);

	// Struct type (aggregate types are not deduped; each block gets its own).
	// Struct-typed members and runtime arrays of structs use laid-out (std430
	// offset-decorated) interior types.
	CK_DYNA uint32_t* member_tids = NULL;
	for (int i = 0; i < (int)asize(member_types); i++) {
		cspv_type* mt = member_types[i];
		if (mt->kind == CSPV_T_STRUCT) {
			apush(member_tids, cspv_laid_struct_tid(ctx, mt, member_lines[i]));
		} else if (mt->kind == CSPV_T_ARRAY && mt->cols == -1 && mt->elem->kind == CSPV_T_STRUCT) {
			apush(member_tids, cspv_laid_runtime_array_tid(ctx, mt->elem, member_lines[i]));
		} else {
			apush(member_tids, cspv_type_id(ctx, mt));
		}
	}
	uint32_t struct_id = cspv_new_id(ctx);
	CK_DYNA uint32_t* w = NULL;
	apush(w, struct_id);
	for (int i = 0; i < (int)asize(member_tids); i++) apush(w, member_tids[i]);
	cspv_emit(&ctx->globals, CSpvOpTypeStruct, w, (int)asize(w));
	afree(w);

	// Offsets: std140 for uniform blocks, std430 for buffer blocks. For the types
	// allowed here the only difference is that std430 does not round array/struct
	// strides up to 16, which cspv_std140_layout's supported set never triggers --
	// except runtime arrays, which are handled explicitly.
	int offset = 0;
	int first_member = (int)asize(ctx->reflection.uniform_members);
	for (int i = 0; i < (int)asize(member_types); i++) {
		int align = 0, size = 0;
		if (member_types[i]->kind == CSPV_T_ARRAY && member_types[i]->cols == -1) {
			int elem_size = 0;
			cspv_std430_layout(ctx, member_types[i]->elem, member_lines[i], &align, &elem_size);
			size = 0; // Runtime-sized; nothing follows (enforced above).
		} else if (is_buffer) {
			cspv_std430_layout(ctx, member_types[i], member_lines[i], &align, &size);
		} else {
			cspv_std140_layout(ctx, member_types[i], member_lines[i], &align, &size);
		}
		offset = (offset + align - 1) / align * align;
		if (!is_buffer) {
			CSPV_ReflectionMember rm;
			rm.name = member_names[i];
			rm.type = cspv_reflect_type(member_types[i]);
			rm.offset = offset;
			rm.array_length = 1;
			apush(ctx->reflection.uniform_members, rm);
		}
		uint32_t deco[4] = { struct_id, (uint32_t)i, CSpvDecorationOffset, (uint32_t)offset };
		cspv_emit(&ctx->decos, CSpvOpMemberDecorate, deco, 4);
		if (member_types[i]->kind == CSPV_T_MAT) {
			uint32_t cm[3] = { struct_id, (uint32_t)i, CSpvDecorationColMajor };
			cspv_emit(&ctx->decos, CSpvOpMemberDecorate, cm, 3);
			uint32_t ms[4] = { struct_id, (uint32_t)i, CSpvDecorationMatrixStride, 16 };
			cspv_emit(&ctx->decos, CSpvOpMemberDecorate, ms, 4);
		}
		uint32_t mn[2] = { struct_id, (uint32_t)i };
		cspv_emit_str(&ctx->names, CSpvOpMemberName, mn, 2, member_names[i]);
		offset += size;
	}
	cspv_emit2(&ctx->decos, CSpvOpDecorate, struct_id, CSpvDecorationBlock);
	cspv_name_id(ctx, struct_id, block_name);
	if (is_buffer && readonly) {
		for (int i = 0; i < (int)asize(member_types); i++) {
			uint32_t nw[3] = { struct_id, (uint32_t)i, CSpvDecorationNonWritable };
			cspv_emit(&ctx->decos, CSpvOpMemberDecorate, nw, 3);
		}
	}

	// Variable.
	int storage = is_buffer ? CSpvStorageStorageBuffer : CSpvStorageUniform;
	uint32_t ptr_id = cspv_new_id(ctx);
	cspv_emit3(&ctx->globals, CSpvOpTypePointer, ptr_id, (uint32_t)storage, struct_id);
	uint32_t var = cspv_new_id(ctx);
	cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_id, var, (uint32_t)storage);
	cspv_emit3(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationDescriptorSet, (uint32_t)layout->set);
	cspv_emit3(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationBinding, (uint32_t)layout->binding);

	if (instance_name) {
		// Named instance: register one symbol whose type is a struct view of the
		// block; field access reuses the regular struct AccessChain path.
		cspv_type* view = (cspv_type*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_type));
		memset(view, 0, sizeof(*view));
		view->kind = CSPV_T_STRUCT;
		view->name = block_name;
		for (int i = 0; i < (int)asize(member_types); i++) {
			apush(view->field_names, member_names[i]);
			apush(view->field_types, member_types[i]);
		}
		// The struct's spirv id must be the block's laid-out struct, not a fresh one.
		map_set(ctx->type_ids, (uint64_t)(uintptr_t)view, struct_id);
		cspv_symbol* sym = cspv_add_symbol(ctx, instance_name, CSPV_SYM_VAR, view, var, storage, line);
		(void)sym;
	} else {
		// Anonymous: members resolve as bare names at global scope.
		for (int i = 0; i < (int)asize(member_types); i++) {
			cspv_symbol* sym = cspv_add_symbol(ctx, member_names[i], CSPV_SYM_BLOCK_MEMBER, member_types[i], var, storage, member_lines[i]);
			sym->member_index = i;
			cspv_type* mt = member_types[i];
			bool laid = mt->kind == CSPV_T_STRUCT || (mt->kind == CSPV_T_ARRAY && mt->cols == -1 && mt->elem->kind == CSPV_T_STRUCT);
			sym->laid_tid = laid ? member_tids[i] : 0;
		}
	}
	afree(member_tids);

	if (is_buffer) {
		CSPV_ReflectionResource res;
		res.name = block_name;
		res.set = layout->set;
		res.binding = layout->binding;
		res.readonly = readonly;
		apush(ctx->reflection.storage_buffers, res);
	} else {
		CSPV_ReflectionBlock rb;
		rb.name = block_name;
		rb.set = layout->set;
		rb.binding = layout->binding;
		rb.size = (offset + 15) / 16 * 16; // Round to vec4, matching std140 block sizing.
		rb.num_members = (int)asize(member_types);
		rb.first_member = first_member;
		apush(ctx->reflection.uniform_blocks, rb);
	}

	afree(member_types);
	afree(member_names);
	afree(member_lines);
}

static void cspv_gen_uniform_opaque(cspv_ctx* ctx, cspv_layout* layout, cspv_type* type, const char* name, bool readonly, bool writeonly, int line)
{
	if (type->kind == CSPV_T_IMAGE2D) {
		if (layout->format < 0) cspv_errorf(ctx, line, "image '%s' requires a format layout qualifier (e.g. rgba8)", name);
		if (layout->format == 6 || layout->format == 7 || layout->format == 9) ctx->needs_extended_formats = true;
		// Resolve the formatless placeholder to the canonical per-format type.
		cspv_type* found = map_get(ctx->image_types, (uint64_t)layout->format);
		if (!found) {
			found = cspv_make_type(ctx, CSPV_T_IMAGE2D, NULL, layout->format, 0);
			map_set(ctx->image_types, (uint64_t)layout->format, found);
		}
		type = found;
	} else if (type->kind != CSPV_T_SAMPLER2D) {
		cspv_errorf(ctx, line, "uniforms outside blocks must be opaque types (sampler2D, image2D); put '%s' in a uniform block", name);
	}
	if (layout->set < 0 || layout->binding < 0) {
		cspv_errorf(ctx, line, "'%s' requires layout(set = N, binding = M)", name);
	}
	uint32_t var = cspv_new_id(ctx);
	uint32_t ptr_tid = cspv_ptr_type_id(ctx, type, CSpvStorageUniformConstant);
	cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_tid, var, CSpvStorageUniformConstant);
	cspv_emit3(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationDescriptorSet, (uint32_t)layout->set);
	cspv_emit3(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationBinding, (uint32_t)layout->binding);
	if (readonly) cspv_emit2(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationNonWritable);
	if (writeonly) cspv_emit2(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationNonReadable);
	cspv_name_id(ctx, var, name);
	cspv_add_symbol(ctx, name, CSPV_SYM_VAR, type, var, CSpvStorageUniformConstant, line);

	CSPV_ReflectionResource res;
	res.name = name;
	res.set = layout->set;
	res.binding = layout->binding;
	res.readonly = readonly;
	if (type->kind == CSPV_T_SAMPLER2D) apush(ctx->reflection.samplers, res);
	else apush(ctx->reflection.storage_images, res);
}

//--------------------------------------------------------------------------------------------------
// Constant expression evaluation (for global initializers).

// Fold a constant scalar expression. Returns the value as (type, bits).
static void cspv_const_fold_scalar(cspv_ctx* ctx, cspv_expr* e, cspv_type** out_type, uint32_t* out_bits)
{
	switch (e->kind) {
	case CSPV_E_FLOAT_LIT: {
		float f = (float)e->u.fval;
		*out_type = ctx->t_float;
		memcpy(out_bits, &f, 4);
		return;
	}
	case CSPV_E_INT_LIT:
		*out_type = ctx->t_int;
		*out_bits = (uint32_t)(int32_t)e->u.ival;
		return;
	case CSPV_E_UINT_LIT:
		*out_type = ctx->t_uint;
		*out_bits = (uint32_t)e->u.ival;
		return;
	case CSPV_E_BOOL_LIT:
		*out_type = ctx->t_bool;
		*out_bits = e->u.bval ? 1 : 0;
		return;
	case CSPV_E_UNARY: {
		if (e->u.un.op != '-') break;
		cspv_type* t = NULL;
		uint32_t bits = 0;
		cspv_const_fold_scalar(ctx, e->u.un.e, &t, &bits);
		if (t == ctx->t_float) {
			float f;
			memcpy(&f, &bits, 4);
			f = -f;
			memcpy(&bits, &f, 4);
		} else {
			bits = (uint32_t)(-(int32_t)bits);
		}
		*out_type = t;
		*out_bits = bits;
		return;
	}
	case CSPV_E_REF: {
		// const global scalars fold to their value.
		cspv_symbol* sym = cspv_find_symbol(ctx, e->u.name);
		if (sym && sym->has_const_value) {
			*out_type = sym->type;
			*out_bits = sym->const_bits;
			return;
		}
		break;
	}
	case CSPV_E_BINARY: {
		int op = e->u.bin.op;
		bool int_only = op == '%' || op == CSPV_P_SHL || op == CSPV_P_SHR || op == '&' || op == '|' || op == '^';
		if (op != '+' && op != '-' && op != '*' && op != '/' && !int_only) break;
		cspv_type* lt = NULL;
		cspv_type* rt = NULL;
		uint32_t lb = 0;
		uint32_t rb = 0;
		cspv_const_fold_scalar(ctx, e->u.bin.l, &lt, &lb);
		cspv_const_fold_scalar(ctx, e->u.bin.r, &rt, &rb);
		if (lt == ctx->t_float || rt == ctx->t_float) {
			if (int_only) cspv_errorf(ctx, e->line, "integer operator on float constants");
			float a = lt == ctx->t_float ? 0 : (float)(int32_t)lb;
			float b = rt == ctx->t_float ? 0 : (float)(int32_t)rb;
			if (lt == ctx->t_float) memcpy(&a, &lb, 4);
			if (rt == ctx->t_float) memcpy(&b, &rb, 4);
			float r = op == '+' ? a + b : op == '-' ? a - b : op == '*' ? a * b : (b != 0 ? a / b : 0);
			*out_type = ctx->t_float;
			memcpy(out_bits, &r, 4);
		} else {
			int32_t a = (int32_t)lb;
			int32_t b = (int32_t)rb;
			int32_t r = 0;
			switch (op) {
			case '+': r = a + b; break;
			case '-': r = a - b; break;
			case '*': r = a * b; break;
			case '/': r = b != 0 ? a / b : 0; break;
			case '%': r = b != 0 ? a % b : 0; break;
			case '&': r = a & b; break;
			case '|': r = a | b; break;
			case '^': r = a ^ b; break;
			case CSPV_P_SHL: r = a << b; break;
			case CSPV_P_SHR: r = a >> b; break;
			}
			*out_type = lt == ctx->t_uint || rt == ctx->t_uint ? ctx->t_uint : ctx->t_int;
			*out_bits = (uint32_t)r;
		}
		return;
	}
	default:
		break;
	}
	cspv_errorf(ctx, e->line, "expected a constant expression");
}

// Convert a folded scalar constant to a wanted scalar type at fold time.
static uint32_t cspv_const_scalar_as(cspv_ctx* ctx, cspv_type* t, uint32_t bits, cspv_type* want, int line)
{
	if (t != want) {
		if (t == ctx->t_int && want == ctx->t_float) {
			float f = (float)(int32_t)bits;
			memcpy(&bits, &f, 4);
		} else if (t == ctx->t_uint && want == ctx->t_float) {
			float f = (float)bits;
			memcpy(&bits, &f, 4);
		} else if ((t == ctx->t_int && want == ctx->t_uint) || (t == ctx->t_uint && want == ctx->t_int)) {
			// Bits carry over.
		} else {
			cspv_errorf(ctx, line, "cannot convert constant to '%s'", cspv_type_name(want));
		}
	}
	return cspv_const_scalar(ctx, want, bits);
}

// Evaluate a constant expression of the given type, returning a constant id
// (OpConstant / OpConstantComposite). Supports scalars, vectors, and arrays.
static uint32_t cspv_const_expr(cspv_ctx* ctx, cspv_expr* e, cspv_type* want)
{
	if (cspv_is_scalar(want)) {
		cspv_type* t = NULL;
		uint32_t bits = 0;
		cspv_const_fold_scalar(ctx, e, &t, &bits);
		return cspv_const_scalar_as(ctx, t, bits, want, e->line);
	}

	if (want->kind == CSPV_T_VEC) {
		if (e->kind != CSPV_E_CALL || cspv_lookup_type(ctx, e->u.call.name) != want) {
			cspv_errorf(ctx, e->line, "constant '%s' initializers must be written as constructors", cspv_type_name(want));
		}
		int argc = (int)asize(e->u.call.args);
		uint32_t comps[4];
		int n = 0;
		if (argc == 1) {
			// Splat.
			uint32_t c = cspv_const_expr(ctx, e->u.call.args[0], want->elem);
			for (int i = 0; i < want->cols; i++) comps[n++] = c;
		} else {
			for (int i = 0; i < argc; i++) {
				if (n >= want->cols) cspv_errorf(ctx, e->line, "too many components");
				comps[n++] = cspv_const_expr(ctx, e->u.call.args[i], want->elem);
			}
			if (n != want->cols) cspv_errorf(ctx, e->line, "wrong number of components for '%s'", cspv_type_name(want));
		}
		uint32_t w[2 + 4];
		int c = 0;
		uint32_t id = cspv_new_id(ctx);
		w[c++] = cspv_type_id(ctx, want);
		w[c++] = id;
		for (int i = 0; i < n; i++) w[c++] = comps[i];
		cspv_emit(&ctx->globals, CSpvOpConstantComposite, w, c);
		return id;
	}

	if (want->kind == CSPV_T_ARRAY) {
		if (e->kind != CSPV_E_CALL || e->u.call.array_size == -1) {
			cspv_errorf(ctx, e->line, "constant array initializers must use the T[](...) constructor form");
		}
		int argc = (int)asize(e->u.call.args);
		if (argc != want->cols) cspv_errorf(ctx, e->line, "array constructor expects %d element(s), got %d", want->cols, argc);
		CK_DYNA uint32_t* w = NULL;
		uint32_t id = cspv_new_id(ctx);
		apush(w, cspv_type_id(ctx, want));
		apush(w, id);
		for (int i = 0; i < argc; i++) {
			apush(w, cspv_const_expr(ctx, e->u.call.args[i], want->elem));
		}
		cspv_emit(&ctx->globals, CSpvOpConstantComposite, w, (int)asize(w));
		afree(w);
		return id;
	}

	cspv_errorf(ctx, e->line, "unsupported constant initializer type '%s'", cspv_type_name(want));
	return 0;
}

//--------------------------------------------------------------------------------------------------
// Struct declarations and global variables.

static void cspv_gen_struct_decl(cspv_ctx* ctx)
{
	int line = ctx->tok.line;
	const char* name = cspv_expect_any_ident(ctx);
	if (cspv_lookup_type(ctx, name)) cspv_errorf(ctx, line, "redefinition of type '%s'", name);

	cspv_type* t = (cspv_type*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_type));
	memset(t, 0, sizeof(*t));
	t->kind = CSPV_T_STRUCT;
	t->name = name;

	cspv_expect_punct(ctx, '{');
	while (!cspv_is_punct(ctx, '}')) {
		if (ctx->tok.kind == CSPV_TOK_EOF) cspv_errorf(ctx, ctx->tok.line, "unexpected end of file inside struct");
		cspv_type* base_ft = cspv_parse_type(ctx);
		base_ft = cspv_parse_array_suffix(ctx, base_ft);
		// Multiple declarators share the base type: `vec2 a, b, c;`.
		do {
			const char* fname = cspv_expect_any_ident(ctx);
			cspv_type* ft = cspv_parse_array_suffix(ctx, base_ft);
			apush(t->field_names, fname);
			apush(t->field_types, ft);
		} while (cspv_accept_punct(ctx, ','));
		cspv_expect_punct(ctx, ';');
	}
	cspv_advance(ctx); // '}'
	cspv_expect_punct(ctx, ';');

	if (asize(t->field_types) == 0) cspv_errorf(ctx, line, "struct '%s' has no fields", name);
	map_set(ctx->type_names, (uint64_t)(uintptr_t)name, t);
}

static void cspv_gen_global_var(cspv_ctx* ctx, bool is_const, cspv_type* type, const char* name, int line)
{
	type = cspv_parse_array_suffix(ctx, type);
	uint32_t init_id = 0;
	bool folded = false;
	uint32_t folded_bits = 0;
	if (cspv_accept_punct(ctx, '=')) {
		cspv_expr* init = cspv_parse_assign(ctx);
		if (is_const && cspv_is_scalar(type)) {
			// Fold const scalars so they can participate in later constant
			// expressions (array sizes, other const initializers).
			cspv_type* t = NULL;
			uint32_t bits = 0;
			cspv_const_fold_scalar(ctx, init, &t, &bits);
			init_id = cspv_const_scalar_as(ctx, t, bits, type, line);
			// Re-fold the conversion for the recorded value.
			folded = true;
			if (t == type) folded_bits = bits;
			else if (t == ctx->t_int && type == ctx->t_float) { float f = (float)(int32_t)bits; memcpy(&folded_bits, &f, 4); }
			else if (t == ctx->t_uint && type == ctx->t_float) { float f = (float)bits; memcpy(&folded_bits, &f, 4); }
			else folded_bits = bits;
		} else {
			init_id = cspv_const_expr(ctx, init, type);
		}
	} else if (is_const) {
		cspv_errorf(ctx, line, "const '%s' requires an initializer", name);
	}
	cspv_expect_punct(ctx, ';');

	uint32_t ptr_tid = cspv_ptr_type_id(ctx, type, CSpvStoragePrivate);
	uint32_t var = cspv_new_id(ctx);
	if (init_id) {
		cspv_emit4(&ctx->globals, CSpvOpVariable, ptr_tid, var, CSpvStoragePrivate, init_id);
	} else {
		cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_tid, var, CSpvStoragePrivate);
	}
	cspv_name_id(ctx, var, name);
	cspv_symbol* sym = cspv_add_symbol(ctx, name, CSPV_SYM_VAR, type, var, CSpvStoragePrivate, line);
	sym->has_const_value = folded;
	sym->const_bits = folded_bits;
}

//--------------------------------------------------------------------------------------------------
// Top-level parse + generate.

static void cspv_gen_top_level(cspv_ctx* ctx)
{
	while (ctx->tok.kind != CSPV_TOK_EOF) {
		if (cspv_accept_punct(ctx, ';')) continue; // Stray semicolon.

		int line = ctx->tok.line;

		if (cspv_accept_ident(ctx, g_cspv_kw.kw_struct)) {
			cspv_gen_struct_decl(ctx);
			continue;
		}
		cspv_layout layout;
		layout.location = layout.set = layout.binding = -1;
		bool has_layout = false;
		bool flat = false;

		// Qualifiers may appear in any order (e.g. `in flat int` and `flat in int`).
		bool is_io = false;
		bool is_input = false;
		bool readonly = false;
		bool writeonly = false;
		for (;;) {
			if (cspv_accept_ident(ctx, g_cspv_kw.kw_layout)) {
				layout = cspv_parse_layout(ctx);
				has_layout = true;
			} else if (cspv_accept_ident(ctx, g_cspv_kw.kw_flat)) {
				flat = true;
			} else if (cspv_accept_ident(ctx, g_cspv_kw.kw_readonly)) {
				readonly = true;
			} else if (cspv_accept_ident(ctx, g_cspv_kw.kw_writeonly)) {
				writeonly = true;
			} else if (cspv_accept_ident(ctx, g_cspv_kw.kw_in)) {
				is_io = true;
				is_input = true;
			} else if (cspv_accept_ident(ctx, g_cspv_kw.kw_out)) {
				is_io = true;
				is_input = false;
			} else {
				break;
			}
		}
		(void)writeonly;

		if (is_io) {
			// `layout(local_size_x = ...) in;` declares the compute workgroup size.
			if (is_input && cspv_accept_punct(ctx, ';')) {
				if (ctx->stage != CSPV_STAGE_COMPUTE) cspv_errorf(ctx, line, "local_size layout requires a compute shader");
				ctx->local_size[0] = layout.local_size[0] > 0 ? layout.local_size[0] : 1;
				ctx->local_size[1] = layout.local_size[1] > 0 ? layout.local_size[1] : 1;
				ctx->local_size[2] = layout.local_size[2] > 0 ? layout.local_size[2] : 1;
				continue;
			}
			cspv_type* type = cspv_parse_type(ctx);
			const char* name = cspv_expect_any_ident(ctx);
			cspv_expect_punct(ctx, ';');
			cspv_gen_global_inout(ctx, &layout, is_input, flat, type, name, line);
			continue;
		}

		if (cspv_accept_ident(ctx, g_cspv_kw.kw_buffer)) {
			const char* block_name = cspv_expect_any_ident(ctx);
			cspv_gen_uniform_block(ctx, &layout, block_name, true, readonly, line);
			continue;
		}

		if (cspv_accept_ident(ctx, g_cspv_kw.kw_uniform)) {
			// `readonly`/`writeonly` may also come between `uniform` and the type.
			for (;;) {
				if (cspv_accept_ident(ctx, g_cspv_kw.kw_readonly)) readonly = true;
				else if (cspv_accept_ident(ctx, g_cspv_kw.kw_writeonly)) writeonly = true;
				else break;
			}
			if (cspv_at_type(ctx)) {
				cspv_type* type = cspv_parse_type(ctx);
				const char* name = cspv_expect_any_ident(ctx);
				cspv_expect_punct(ctx, ';');
				cspv_gen_uniform_opaque(ctx, &layout, type, name, readonly, writeonly, line);
			} else {
				const char* block_name = cspv_expect_any_ident(ctx);
				cspv_gen_uniform_block(ctx, &layout, block_name, false, false, line);
			}
			continue;
		}

		// Compute shared memory: `shared type name;`.
		if (cspv_accept_ident(ctx, g_cspv_kw.kw_shared)) {
			if (ctx->stage != CSPV_STAGE_COMPUTE) cspv_errorf(ctx, line, "'shared' variables require a compute shader");
			cspv_type* type = cspv_parse_type(ctx);
			const char* name = cspv_expect_any_ident(ctx);
			type = cspv_parse_array_suffix(ctx, type);
			cspv_expect_punct(ctx, ';');
			uint32_t ptr_tid = cspv_ptr_type_id(ctx, type, CSpvStorageWorkgroup);
			uint32_t var = cspv_new_id(ctx);
			cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_tid, var, CSpvStorageWorkgroup);
			cspv_name_id(ctx, var, name);
			cspv_add_symbol(ctx, name, CSPV_SYM_VAR, type, var, CSpvStorageWorkgroup, line);
			continue;
		}

		if (has_layout || flat) {
			// (is_io handled above; reaching here means qualifiers preceded something
			// that is not an in/out/uniform declaration.)
			cspv_errorf(ctx, line, "layout qualifiers must be followed by 'in', 'out', or 'uniform'");
		}

		// Function definition `type name(params) { ... }` or global variable
		// `[const] type name[N]? = init;`.
		bool is_const = cspv_accept_ident(ctx, g_cspv_kw.kw_const);
		if (!cspv_at_type(ctx) && !cspv_is_ident(ctx, g_cspv_kw.kw_void)) {
			cspv_errorf(ctx, line, "unexpected token at global scope");
		}
		cspv_type* ret = cspv_accept_ident(ctx, g_cspv_kw.kw_void) ? ctx->t_void : cspv_parse_type(ctx);
		const char* name = cspv_expect_any_ident(ctx);
		if (!cspv_is_punct(ctx, '(')) {
			if (ret == ctx->t_void) cspv_errorf(ctx, line, "variables cannot be 'void'");
			cspv_gen_global_var(ctx, is_const, ret, name, line);
			continue;
		}
		if (is_const) cspv_errorf(ctx, line, "'const' is not valid on a function");
		cspv_expect_punct(ctx, '(');
		CK_DYNA cspv_param* params = NULL;
		if (!cspv_is_punct(ctx, ')')) {
			// `void` as an empty parameter list.
			if (cspv_is_ident(ctx, g_cspv_kw.kw_void) && cspv_peek(ctx, 1)->kind == CSPV_TOK_PUNCT && cspv_peek(ctx, 1)->punct == ')') {
				cspv_advance(ctx);
			} else {
				do {
					cspv_param p;
					p.qual = 0;
					if (cspv_accept_ident(ctx, g_cspv_kw.kw_in)) p.qual = 0;
					else if (cspv_accept_ident(ctx, g_cspv_kw.kw_out)) p.qual = 1;
					else if (cspv_accept_ident(ctx, g_cspv_kw.kw_inout)) p.qual = 2;
					p.type = cspv_parse_type(ctx);
					p.type = cspv_parse_array_suffix(ctx, p.type); // `vec2[8] v` form.
					p.name = cspv_expect_any_ident(ctx);
					p.type = cspv_parse_array_suffix(ctx, p.type); // `vec2 v[8]` form.
					apush(params, p);
				} while (cspv_accept_punct(ctx, ','));
			}
		}
		cspv_expect_punct(ctx, ')');
		cspv_stmt* body = cspv_parse_block(ctx);
		cspv_gen_function(ctx, ret, name, params, body, line);
		afree(params);
	}
}

//--------------------------------------------------------------------------------------------------
// Context setup.

static cspv_type* cspv_make_type(cspv_ctx* ctx, cspv_type_kind kind, cspv_type* elem, int cols, int rows)
{
	cspv_type* t = (cspv_type*)cspv_arena_alloc(&ctx->arena, sizeof(cspv_type));
	memset(t, 0, sizeof(*t));
	t->kind = kind;
	t->elem = elem;
	t->cols = cols;
	t->rows = rows;
	return t;
}

static void cspv_register_type(cspv_ctx* ctx, const char* name, cspv_type* type)
{
	map_set(ctx->type_names, (uint64_t)(uintptr_t)sintern(name), type);
}

static void cspv_init_types(cspv_ctx* ctx)
{
	ctx->t_void = cspv_make_type(ctx, CSPV_T_VOID, NULL, 0, 0);
	ctx->t_bool = cspv_make_type(ctx, CSPV_T_BOOL, NULL, 0, 0);
	ctx->t_int = cspv_make_type(ctx, CSPV_T_INT, NULL, 0, 0);
	ctx->t_uint = cspv_make_type(ctx, CSPV_T_UINT, NULL, 0, 0);
	ctx->t_float = cspv_make_type(ctx, CSPV_T_FLOAT, NULL, 0, 0);
	for (int i = 0; i < 3; i++) {
		ctx->t_vec[i] = cspv_make_type(ctx, CSPV_T_VEC, ctx->t_float, i + 2, 0);
		ctx->t_ivec[i] = cspv_make_type(ctx, CSPV_T_VEC, ctx->t_int, i + 2, 0);
		ctx->t_uvec[i] = cspv_make_type(ctx, CSPV_T_VEC, ctx->t_uint, i + 2, 0);
		ctx->t_bvec[i] = cspv_make_type(ctx, CSPV_T_VEC, ctx->t_bool, i + 2, 0);
		ctx->t_mat[i] = cspv_make_type(ctx, CSPV_T_MAT, ctx->t_float, i + 2, i + 2);
	}
	ctx->t_sampler2d = cspv_make_type(ctx, CSPV_T_SAMPLER2D, ctx->t_float, 0, 0);
	ctx->t_usampler2d = cspv_make_type(ctx, CSPV_T_SAMPLER2D, ctx->t_uint, 0, 0);

	cspv_register_type(ctx, "bool", ctx->t_bool);
	cspv_register_type(ctx, "int", ctx->t_int);
	cspv_register_type(ctx, "uint", ctx->t_uint);
	cspv_register_type(ctx, "float", ctx->t_float);
	cspv_register_type(ctx, "vec2", ctx->t_vec[0]);
	cspv_register_type(ctx, "vec3", ctx->t_vec[1]);
	cspv_register_type(ctx, "vec4", ctx->t_vec[2]);
	cspv_register_type(ctx, "ivec2", ctx->t_ivec[0]);
	cspv_register_type(ctx, "ivec3", ctx->t_ivec[1]);
	cspv_register_type(ctx, "ivec4", ctx->t_ivec[2]);
	cspv_register_type(ctx, "uvec2", ctx->t_uvec[0]);
	cspv_register_type(ctx, "uvec3", ctx->t_uvec[1]);
	cspv_register_type(ctx, "uvec4", ctx->t_uvec[2]);
	cspv_register_type(ctx, "bvec2", ctx->t_bvec[0]);
	cspv_register_type(ctx, "bvec3", ctx->t_bvec[1]);
	cspv_register_type(ctx, "bvec4", ctx->t_bvec[2]);
	cspv_register_type(ctx, "sampler2D", ctx->t_sampler2d);
	cspv_register_type(ctx, "usampler2D", ctx->t_usampler2d);
	cspv_register_type(ctx, "mat2", ctx->t_mat[0]);
	cspv_register_type(ctx, "mat3", ctx->t_mat[1]);
	cspv_register_type(ctx, "mat4", ctx->t_mat[2]);
	// Formatless placeholder; resolved to a per-format canonical type at the
	// uniform declaration (which carries the layout format qualifier).
	cspv_register_type(ctx, "image2D", cspv_make_type(ctx, CSPV_T_IMAGE2D, NULL, -1, 0));
}

static void cspv_add_builtin_input(cspv_ctx* ctx, const char* name, cspv_type* type, int builtin)
{
	uint32_t ptr_tid = cspv_ptr_type_id(ctx, type, CSpvStorageInput);
	uint32_t var = cspv_new_id(ctx);
	cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_tid, var, CSpvStorageInput);
	cspv_emit3(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationBuiltIn, (uint32_t)builtin);
	cspv_name_id(ctx, var, name);
	cspv_add_symbol(ctx, sintern(name), CSPV_SYM_VAR, type, var, CSpvStorageInput, 0);
	apush(ctx->interface_ids, var);
}

// Compute-stage builtin inputs, created eagerly (unused ones are harmless).
static void cspv_init_compute_builtins(cspv_ctx* ctx)
{
	static const struct { const char* name; int builtin; bool is_index; } builtins[] = {
		{ "gl_GlobalInvocationID", 28, false },
		{ "gl_LocalInvocationID", 27, false },
		{ "gl_WorkGroupID", 26, false },
		{ "gl_NumWorkGroups", 24, false },
		{ "gl_LocalInvocationIndex", 29, true },
	};
	for (int i = 0; i < (int)(sizeof(builtins) / sizeof(builtins[0])); i++) {
		cspv_type* type = builtins[i].is_index ? ctx->t_uint : ctx->t_uvec[1];
		uint32_t ptr_tid = cspv_ptr_type_id(ctx, type, CSpvStorageInput);
		uint32_t var = cspv_new_id(ctx);
		cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_tid, var, CSpvStorageInput);
		cspv_emit3(&ctx->decos, CSpvOpDecorate, var, CSpvDecorationBuiltIn, (uint32_t)builtins[i].builtin);
		cspv_name_id(ctx, var, builtins[i].name);
		cspv_add_symbol(ctx, sintern(builtins[i].name), CSPV_SYM_VAR, type, var, CSpvStorageInput, 0);
		apush(ctx->interface_ids, var);
	}
}

// Emit the gl_PerVertex output block for vertex shaders (only member: gl_Position).
static void cspv_init_gl_pervertex(cspv_ctx* ctx)
{
	uint32_t vec4_tid = cspv_type_id(ctx, ctx->t_vec[2]);
	uint32_t struct_id = cspv_new_id(ctx);
	cspv_emit2(&ctx->globals, CSpvOpTypeStruct, struct_id, vec4_tid);
	cspv_emit2(&ctx->decos, CSpvOpDecorate, struct_id, CSpvDecorationBlock);
	uint32_t deco[4] = { struct_id, 0, CSpvDecorationBuiltIn, CSpvBuiltInPosition };
	cspv_emit(&ctx->decos, CSpvOpMemberDecorate, deco, 4);
	cspv_name_id(ctx, struct_id, "gl_PerVertex");
	uint32_t mn[2] = { struct_id, 0 };
	cspv_emit_str(&ctx->names, CSpvOpMemberName, mn, 2, "gl_Position");

	uint32_t ptr_id = cspv_new_id(ctx);
	cspv_emit3(&ctx->globals, CSpvOpTypePointer, ptr_id, CSpvStorageOutput, struct_id);
	ctx->gl_pervertex_var = cspv_new_id(ctx);
	cspv_emit3(&ctx->globals, CSpvOpVariable, ptr_id, ctx->gl_pervertex_var, CSpvStorageOutput);
	apush(ctx->interface_ids, ctx->gl_pervertex_var);

	cspv_symbol* sym = cspv_add_symbol(ctx, g_cspv_kw.kw_gl_Position, CSPV_SYM_GL_POSITION, ctx->t_vec[2], ctx->gl_pervertex_var, CSpvStorageOutput, 0);
	(void)sym;
}

//--------------------------------------------------------------------------------------------------
// Module assembly.

static uint32_t* cspv_assemble(cspv_ctx* ctx, size_t* out_word_count)
{
	CK_DYNA uint32_t* mod = NULL;

	// Header.
	apush(mod, CSPV_MAGIC);
	apush(mod, CSPV_VERSION);
	apush(mod, 0);            // Generator id (0: no registered tool id).
	apush(mod, ctx->next_id); // Id bound.
	apush(mod, 0);            // Schema.

	// OpCapability Shader.
	cspv_emit1(&mod, CSpvOpCapability, 1);
	if (ctx->needs_image_query) cspv_emit1(&mod, CSpvOpCapability, 50); // ImageQuery
	if (ctx->needs_extended_formats) cspv_emit1(&mod, CSpvOpCapability, 49); // StorageImageExtendedFormats

	// OpExtInstImport %glsl "GLSL.std.450".
	cspv_emit_str(&mod, CSpvOpExtInstImport, &ctx->glsl_ext_id, 1, "GLSL.std.450");

	// OpMemoryModel Logical GLSL450.
	cspv_emit2(&mod, CSpvOpMemoryModel, 0, 1);

	// OpEntryPoint <model> %main "main" <interface...>.
	{
		uint32_t model = 0; // Vertex
		if (ctx->stage == CSPV_STAGE_FRAGMENT) model = 4;
		else if (ctx->stage == CSPV_STAGE_COMPUTE) model = 5;
		CK_DYNA uint32_t* w = NULL;
		apush(w, model);
		apush(w, ctx->entry_func_id);
		cspv_push_string(&w, "main");
		for (int i = 0; i < (int)asize(ctx->interface_ids); i++) {
			apush(w, ctx->interface_ids[i]);
		}
		cspv_emit(&mod, CSpvOpEntryPoint, w, (int)asize(w));
		afree(w);
	}

	if (ctx->stage == CSPV_STAGE_FRAGMENT) {
		// OpExecutionMode %main OriginUpperLeft.
		cspv_emit2(&mod, CSpvOpExecutionMode, ctx->entry_func_id, 7);
	} else if (ctx->stage == CSPV_STAGE_COMPUTE) {
		// OpExecutionMode %main LocalSize x y z.
		uint32_t w[5] = { ctx->entry_func_id, 17, (uint32_t)ctx->local_size[0], (uint32_t)ctx->local_size[1], (uint32_t)ctx->local_size[2] };
		cspv_emit(&mod, CSpvOpExecutionMode, w, 5);
	}

	for (int i = 0; i < (int)asize(ctx->names); i++) apush(mod, ctx->names[i]);
	for (int i = 0; i < (int)asize(ctx->decos); i++) apush(mod, ctx->decos[i]);
	for (int i = 0; i < (int)asize(ctx->globals); i++) apush(mod, ctx->globals[i]);
	for (int i = 0; i < (int)asize(ctx->funcs); i++) apush(mod, ctx->funcs[i]);

	// Hand the ckit dynamic array to the caller (freed by cspv_free via afree).
	*out_word_count = asize(mod);
	return mod;
}

//--------------------------------------------------------------------------------------------------
// Cleanup.

static void cspv_cleanup(cspv_ctx* ctx)
{
	afree(ctx->names);
	afree(ctx->decos);
	afree(ctx->globals);
	afree(ctx->funcs);
	afree(ctx->body);
	afree(ctx->local_vars);
	afree(ctx->interface_ids);
	map_free(ctx->type_ids);
	map_free(ctx->ptr_type_ids);
	map_free(ctx->const_ids);
	map_free(ctx->fn_type_ids);
	map_free(ctx->type_names);
	for (int i = 0; i < (int)map_size(ctx->functions); i++) {
		for (cspv_symbol* o = map_val(ctx->functions, i); o; o = o->next_overload) {
			afree(o->params);
			afree(o->param_quals);
		}
	}
	map_free(ctx->functions);
	while (asize(ctx->scopes)) cspv_pop_scope(ctx);
	afree(ctx->scopes);
	afree(ctx->files);
	sfree(ctx->pp_text);
	afree(ctx->reflection.samplers);
	afree(ctx->reflection.storage_images);
	afree(ctx->reflection.storage_buffers);
	afree(ctx->reflection.uniform_blocks);
	afree(ctx->reflection.uniform_members);
	afree(ctx->reflection.inputs);
	cspv_arena_free(&ctx->arena);
}

//--------------------------------------------------------------------------------------------------
// Public API.

CSPV_Result cspv_compile_ex(const char* source, CSPV_Stage stage, const CSPV_Options* opts)
{
	CSPV_Result result;
	memset(&result, 0, sizeof(result));

	cspv_init_keywords();
	cspv_init_intrins();

	cspv_ctx ctx_storage;
	cspv_ctx* ctx = &ctx_storage;
	memset(ctx, 0, sizeof(*ctx));
	ctx->stage = stage;
	ctx->line = 1;
	ctx->next_id = 1;

	if (setjmp(ctx->jmp)) {
		result.success = false;
		result.error_message = ctx->error;
		cspv_cleanup(ctx);
		return result;
	}

	ctx->pp_text = cspv_preprocess(ctx, source, opts);
	ctx->p = ctx->pp_text;
	ctx->line = 1;
	ctx->file_index = 0;

	if (opts && opts->preprocess_only) {
		result.success = true;
		result.preprocessed = ctx->pp_text;
		ctx->pp_text = NULL;
		cspv_cleanup(ctx);
		return result;
	}

	ctx->glsl_ext_id = cspv_new_id(ctx);
	cspv_init_types(ctx);
	cspv_push_scope(ctx); // Global scope.
	if (stage == CSPV_STAGE_VERTEX) {
		cspv_init_gl_pervertex(ctx);
		cspv_add_builtin_input(ctx, "gl_VertexIndex", ctx->t_int, 42);   // BuiltIn VertexIndex
		cspv_add_builtin_input(ctx, "gl_InstanceIndex", ctx->t_int, 43); // BuiltIn InstanceIndex
	} else if (stage == CSPV_STAGE_FRAGMENT) {
		cspv_add_builtin_input(ctx, "gl_FragCoord", ctx->t_vec[2], 15);  // BuiltIn FragCoord
	} else if (stage == CSPV_STAGE_COMPUTE) {
		cspv_init_compute_builtins(ctx);
	}

	cspv_advance(ctx); // Prime the first token.
	cspv_gen_top_level(ctx);

	if (!ctx->entry_func_id) {
		cspv_errorf(ctx, 0, "no 'void main()' entry point defined");
	}
	if (stage == CSPV_STAGE_COMPUTE && ctx->local_size[0] == 0) {
		cspv_errorf(ctx, 0, "compute shaders must declare layout(local_size_x = ..., ...) in;");
	}

	result.spirv = cspv_assemble(ctx, &result.word_count);
	result.success = true;
	result.reflection = ctx->reflection;
	memset(&ctx->reflection, 0, sizeof(ctx->reflection)); // Ownership moved to the result.
	if (opts && opts->return_preprocessed) {
		result.preprocessed = ctx->pp_text;
		ctx->pp_text = NULL;
	}
	cspv_cleanup(ctx);
	return result;
}

CSPV_Result cspv_compile(const char* source, CSPV_Stage stage)
{
	return cspv_compile_ex(source, stage, NULL);
}

void cspv_free(CSPV_Result* result)
{
	sfree(result->error_message);
	afree(result->spirv);
	afree(result->reflection.samplers);
	afree(result->reflection.storage_images);
	afree(result->reflection.storage_buffers);
	afree(result->reflection.uniform_blocks);
	afree(result->reflection.uniform_members);
	afree(result->reflection.inputs);
	sfree(result->preprocessed);
	memset(result, 0, sizeof(*result));
}

#endif // CUTE_SPIRV_IMPLEMENTATION_ONCE
#endif // CUTE_SPIRV_IMPLEMENTATION

/*
	------------------------------------------------------------------------------
	This software is available under 2 licenses - you may choose the one you like.
	------------------------------------------------------------------------------
	ALTERNATIVE A - zlib license
	Copyright (c) 2026 Randy Gaul https://randygaul.github.io/
	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
	------------------------------------------------------------------------------
	ALTERNATIVE B - Public Domain (www.unlicense.org)
	This is free and unencumbered software released into the public domain.
	Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
	software, either in source code form or as a compiled binary, for any purpose,
	commercial or non-commercial, and by any means.
	In jurisdictions that recognize copyright laws, the author or authors of this
	software dedicate any and all copyright interest in the software to the public
	domain. We make this dedication for the benefit of the public at large and to
	the detriment of our heirs and successors. We intend this dedication to be an
	overt act of relinquishment in perpetuity of all present and future rights to
	this software under copyright law.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	------------------------------------------------------------------------------
*/
