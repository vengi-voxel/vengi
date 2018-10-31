/**
 * @file
 * @brief Condition related stuff
 * @defgroup Condition
 * @{
 * @sa ai{ICondition}
 */

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <memory>

#include "common/MemoryAllocator.h"
#include "common/Thread.h"

#include "AIFactories.h"

namespace ai {

class AI;
typedef std::shared_ptr<AI> AIPtr;

/**
 * @brief Macro to simplify the condition creation. Just give the class name of the condition as parameter.
 */
#define CONDITION_CLASS(ConditionName) \
	explicit ConditionName(const std::string& parameters = "") : \
		::ai::ICondition(#ConditionName, parameters) { \
	} \
public: \
	virtual ~ConditionName() { \
	}

/**
 * @brief A condition factory macro to ease and unify the registration at AIRegistry.
 * You still have to implement the Factory::create method.
 */
#define CONDITION_FACTORY_NO_IMPL(ConditionName) \
public: \
	class Factory: public ::ai::IConditionFactory { \
	public: \
		::ai::ConditionPtr create (const ::ai::ConditionFactoryContext *ctx) const override; \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

/**
 * @brief A condition factory macro to ease and unify the registration at @ai{AIRegistry}.
 */
#define CONDITION_FACTORY(ConditionName) \
public: \
	class Factory: public ::ai::IConditionFactory { \
	public: \
		::ai::ConditionPtr create (const ::ai::ConditionFactoryContext *ctx) const override { \
			return std::make_shared<ConditionName>(ctx->parameters); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

/**
 * @brief A condition factory singleton macro to ease and unify the registration at @ai{AIRegistry}.
 * Nothing from the given context is taken, if you need this, use the instance based factory,
 * not the singleton based.
 */
#define CONDITION_FACTORY_SINGLETON \
public: \
	class Factory: public ::ai::IConditionFactory { \
		::ai::ConditionPtr create (const ::ai::ConditionFactoryContext */*ctx*/) const { \
			return get(); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

/**
 * @brief Macro to create a singleton conditions for very easy conditions without a state.
 */
#define CONDITION_CLASS_SINGLETON(ConditionName) \
private: \
	CONDITION_CLASS(ConditionName) \
public: \
	static ConditionPtr& get() { \
		AI_THREAD_LOCAL ConditionName* c = nullptr; \
		if (c == nullptr) { c = new ConditionName; } \
		AI_THREAD_LOCAL ConditionPtr _instance(c); \
		return _instance; \
	} \
	CONDITION_FACTORY_SINGLETON

#define CONDITION_PRINT_SUBCONDITIONS_GETCONDITIONNAMEWITHVALUE \
	void getConditionNameWithValue(std::stringstream& s, const ::ai::AIPtr& entity) override { \
		bool first = true; \
		s << "("; \
		for (::ai::ConditionsConstIter i = _conditions.begin(); i != _conditions.end(); ++i) { \
			if (!first) { \
				s << ","; \
			} \
			s << (*i)->getNameWithConditions(entity); \
			first = false; \
		} \
		s << ")"; \
	}

class ICondition;
typedef std::shared_ptr<ICondition> ConditionPtr;
typedef std::vector<ConditionPtr> Conditions;
typedef Conditions::iterator ConditionsIter;
typedef Conditions::const_iterator ConditionsConstIter;

/**
 * @brief A condition can be placed on a @ai{TreeNode} to decide which node is going to get executed. In general they are stateless.
 * If they are not, it should explicitly get noted.
 */
class ICondition : public MemObject {
protected:
	static int getNextId() {
		static int _nextId = 0;
		const int id = _nextId++;
		return id;
	}

	/**
	 * @brief Every node has an id to identify it. It's unique per type.
	 */
	int _id;
	const std::string _name;
	const std::string _parameters;

	/**
	 * @brief Override this method to get a more detailed result in @c getNameWithConditions()
	 *
	 * @param[out] s The string stream to write your details to
	 * @param[in,out] entity The entity that is used to evaluate a condition
	 * @sa getNameWithConditions()
	 */
	virtual void getConditionNameWithValue(std::stringstream& s, const AIPtr& entity) {
		(void)entity;
		s << "{" << _parameters << "}";
	}
public:
	ICondition(const std::string& name, const std::string& parameters) :
			_id(getNextId()), _name(name), _parameters(parameters) {
	}

	virtual ~ICondition() {
	}

	/**
	 * @brief Checks whether the condition evaluates to @c true for the given @c entity.
	 * @param[in,out] entity The entity that is used to evaluate the condition
	 * @return @c true if the condition is fulfilled, @c false otherwise.
	 */
	virtual bool evaluate(const AIPtr& entity) = 0;

	/**
	 * @brief Returns the short name of the condition - without any related conditions or results.
	 */
	const std::string& getName() const;

	/**
	 * @brief Returns the raw parameters of the condition
	 */
	const std::string& getParameters() const;

	/**
	 * @brief Returns the full condition string with all related conditions and results of the evaluation method
	 * @param[in,out] entity The entity that is used to evaluate the condition
	 * @sa getConditionNameWithValue()
	 */
	inline std::string getNameWithConditions(const AIPtr& entity) {
		std::stringstream s;
		s << getName();
		getConditionNameWithValue(s, entity);
		s << "[";
		s << (evaluate(entity) ? "1" : "0");
		s << "]";
		return s.str();
	}
};

inline const std::string& ICondition::getName() const {
	return _name;
}

inline const std::string& ICondition::getParameters() const {
	return _parameters;
}

}

/**
 * @}
 */
