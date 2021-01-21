/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#include "core/concurrent/Lock.h"
#include "backend/entity/ai/common/Math.h"
#include "backend/entity/ai/ICharacter.h"
#include "backend/entity/ai/AI.h"
#include <memory>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace backend {

/**
 * @brief Maintains the groups a @c AI can be in.
 * @note Keep in mind that if you destroy an @c AI somewhere in the game, to also
 * remove it from the groups.
 *
 * Every @ai{Zone} has its own @c GroupMgr instance. It is automatically updated with the zone.
 * The average group position is only updated once per @c update() call.
 */
class GroupMgr {
private:
	typedef std::unordered_set<AIPtr> GroupMembersSet;
	typedef GroupMembersSet::iterator GroupMembersSetIter;
	typedef GroupMembersSet::const_iterator GroupMembersSetConstIter;

	core_trace_mutex(core::Lock, _groupLock, "GroupLock");
	struct Group {
		AIPtr leader;
		GroupMembersSet members;
		glm::vec3 position;
	};

	typedef std::unordered_multimap<AIPtr, GroupId> GroupMembers;
	typedef std::unordered_map<GroupId, Group> Groups;
	typedef Groups::const_iterator GroupsConstIter;
	typedef Groups::iterator GroupsIter;

	core_trace_mutex(core::Lock, _lock, "GroupMgr");
	Groups _groups core_thread_guarded_by(_lock);
	GroupMembers _groupMembers core_thread_guarded_by(_lock);

public:
	GroupMgr () {
	}
	virtual ~GroupMgr () {
	}

	/**
	 * @brief Adds a new group member to the given @ai{GroupId}. If the group does not yet
	 * exists, it it created and the given @ai{AI} instance will be the leader of the
	 * group.
	 *
	 * @sa remove()
	 *
	 * @param ai The @ai{AI} to add to the group. Keep
	 * in mind that you have to remove it manually from any group
	 * whenever you destroy the @ai{AI} instance.
	 * @return @c true if the add to the group was successful.
	 *
	 * @note This method performs a write lock on the group manager
	 */
	bool add(GroupId id, const AIPtr& ai);

	void update(int64_t deltaTime);

	/**
	 * @brief Removes a group member from the given @ai{GroupId}. If the member
	 * is the group leader, a new leader will be picked randomly. If after the
	 * removal of the member only one other member is left in the group, the
	 * group is destroyed.
	 *
	 * @param ai The @ai{AI} to remove from this the group.
	 * @return @c true if the given @ai{AI} was removed from the group,
	 * @c false if the removal failed (e.g. the @ai{AI} instance was not part of
	 * the group)
	 *
	 * @note This method performs a write lock on the group manager
	 */
	bool remove(GroupId id, const AIPtr& ai);

	/**
	 * @brief Use this method to remove a @ai{AI} instance from all the group it is
	 * part of. Useful if you e.g. destroy a @ai{AI} instance.
	 *
	 * @note This method performs a write lock on the group manager
	 */
	bool removeFromAllGroups(const AIPtr& ai);

	/**
	 * @brief Returns the average position of the group
	 *
	 * @note If the given group doesn't exist or some other error occurred, this method returns @c glm::vec3::VEC3_INFINITE
	 * @note The position of a group is calculated once per @c update() call.
	 *
	 * @note This method performs a read lock on the group manager
	 */
	bool getPosition(GroupId id, glm::vec3& position) const;

	/**
	 * @return The @ai{ICharacter} object of the leader, or @c nullptr if no such group exists.
	 *
	 * @note This method performs a read lock on the group manager
	 */
	AIPtr getLeader(GroupId id) const;

	/**
	 * @brief Visit all the group members of the given group until the functor returns @c false
	 *
	 * @note This methods performs a read lock on the group manager
	 */
	template<typename Func>
	void visit(GroupId id, Func& func) const {
		core::ScopedLock scopedLock(_lock);
		const GroupsConstIter& i = _groups.find(id);
		if (i == _groups.end()) {
			return;
		}
		for (GroupMembersSetConstIter it = i->second.members.begin(); it != i->second.members.end(); ++it) {
			const AIPtr& chr = *it;
			if (!func(chr))
				break;
		}
	}

	/**
	 * @return If the group doesn't exist, this method returns @c 0 - otherwise the amount of members
	 * that must be bigger than @c 1
	 *
	 * @note This method performs a read lock on the group manager
	 */
	int getGroupSize(GroupId id) const;

	/**
	 * @note This method performs a read lock on the group manager
	 */
	bool isInAnyGroup(const AIPtr& ai) const;

	/**
	 * @note This method performs a read lock on the group manager
	 */
	bool isInGroup(GroupId id, const AIPtr& ai) const;

	/**
	 * @note This method performs a read lock on the group manager
	 */
	bool isGroupLeader(GroupId id, const AIPtr& ai) const;
};

}
