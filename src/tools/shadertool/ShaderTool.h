/**
 * @file
 */

#pragma once

#include "core/ConsoleApp.h"
#include "Util.h"
#include <simplecpp.h>

class TokenIterator {
private:
	const simplecpp::TokenList* _tokenList = nullptr;
	const simplecpp::Token *_tok = nullptr;
public:
	void init(const simplecpp::TokenList* tokenList) {
		_tokenList = tokenList;
		_tok = _tokenList->cfront();
	}

	inline bool hasNext() const {
		return _tok != nullptr;
	}

	inline std::string next() {
		const std::string& token = _tok->str;
		_tok = _tok->next;
		return token;
	}

	inline std::string prev() {
		_tok = _tok->previous;
		return _tok->str;
	}

	inline int line() const {
		if (!_tok) {
			return -1;
		}
		return _tok->location.line;
	}

	inline std::string peekNext() const {
		if (!_tok) {
			return "";
		}
		return _tok->str;
	}
};

/**
 * @brief This tool validates the shaders and generated c++ code for them.
 */
class ShaderTool: public core::ConsoleApp {
private:
	using Super = core::ConsoleApp;
protected:
	ShaderStruct _shaderStruct;
	TokenIterator _tok;
	Layout _layout;
	std::string _namespaceSrc;
	std::string _sourceDirectory;
	std::string _shaderDirectory;
	std::string _shaderTemplateFile;
	std::string _uniformBufferTemplateFile;
	std::string _shaderfile;

	bool parseLayout();
	bool parse(const std::string& src, bool vertex);

	std::string typeAlign(const Variable& v) const;
	size_t typeSize(const Variable& v) const;
	std::string typePadding(const Variable& v, int& padding) const;

	void generateSrc();
public:
	ShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onConstruct() override;
	core::AppState onRunning() override;
};
