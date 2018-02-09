/**
 * @file
 */

#pragma once

#include <vector>

namespace persistence {

class Model;

/**
 * @brief Interface used in combination with @c PersistenceMgr to do mass updates on dirty
 * states
 *
 * @see @c LongCounter For use in relative updates
 */
class ISavable {
protected:
	using Models = std::vector<const Model*>;
public:
	virtual ~ISavable() {}

	/**
	 * @brief Returns pointers to the @c Model instances that you are about to push
	 * to the database.
	 * It's important to note that this is going to be executed in a mass query. Thus you
	 * have to make sure that the returned Models have the same values set. Always! Usually
	 * you would make the models members of the handler that inherits from @c ISavable and
	 * just return the pointers the these members. The data inside the models is not modified.
	 * You won't get auto generated fields back into the @c Model instances. You should not
	 * operate on the models outside of this method.
	 * @note This is called in an own thread - make sure you synchronize this.
	 * @return A list of pointers to @c Model instances. The memory ownership stays at this
	 * object. Might also return @c false if there is nothing to persist at the moment, @c true
	 * if @c Model pointers were added to the list
	 * @note To delete models, you have to mark the model as such via @c Model::flagForDelete()
	 */
	virtual bool getDirtyModels(Models& models) = 0;
};

}
