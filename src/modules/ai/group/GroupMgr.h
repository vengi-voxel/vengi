/**
 * @file
 */
#pragma once

#include "common/Thread.h"
#include "common/Types.h"
#include "common/Math.h"
#include "ICharacter.h"
#include "AI.h"
#include <map>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace ai {

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
	struct AveragePositionFunctor {
		glm::vec3 operator()(const glm::vec3& result, const AIPtr& ai) {
			return ai->getCharacter()->getPosition() + result;
		}
	};

	typedef std::unordered_set<AIPtr> GroupMembersSet;
	typedef GroupMembersSet::iterator GroupMembersSetIter;
	typedef GroupMembersSet::const_iterator GroupMembersSetConstIter;

	ReadWriteLock _groupLock = {"groupmgr-group"};
	struct Group {
		AIPtr leader;
		GroupMembersSet members;
		glm::vec3 position;
	};

	typedef std::unordered_multimap<AIPtr, GroupId> GroupMembers;
	typedef std::unordered_map<GroupId, Group> Groups;
	typedef Groups::const_iterator GroupsConstIter;
	typedef Groups::iterator GroupsIter;

	GroupMembersSet _empty;
	ReadWriteLock _lock = {"groupmgr"};
	Groups _groups;
	GroupMembers _groupMembers;

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
	 * @note If the given group doesn't exist or some other error occurred, this method returns @c glm::vec3::INFINITE
	 * @note The position of a group is calculated once per @c update() call.
	 *
	 * @note This method performs a read lock on the group manager
	 */
	glm::vec3 getPosition(GroupId id) const;

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
		ScopedReadLock scopedLock(_lock);
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


inline void GroupMgr::update(int64_t) {
	ScopedReadLock scopedLock(_lock);
	for (auto i = _groups.begin(); i != _groups.end(); ++i) {
		Group& group = i->second;
		glm::vec3 averagePosition;
		{
			ScopedReadLock lock(_groupLock);
			averagePosition = std::accumulate(group.members.begin(), group.members.end(), glm::vec3(), AveragePositionFunctor());
			averagePosition *= 1.0f / (float) group.members.size();
		}
		ScopedWriteLock lock(_groupLock);
		group.position = averagePosition;
	}
}

inline bool GroupMgr::add(GroupId id, const AIPtr& ai) {
	ScopedWriteLock scopedLock(_lock);
	GroupsIter i = _groups.find(id);
	if (i == _groups.end()) {
		Group group;
		group.leader = ai;
		i = _groups.insert(std::pair<GroupId, Group>(id, group)).first;
	}

	Group& group = i->second;
	ScopedWriteLock lock(_groupLock);
	std::pair<GroupMembersSetIter, bool> ret = group.members.insert(ai);
	if (ret.second) {
		_groupMembers.insert(GroupMembers::value_type(ai, id));
		return true;
	}
	return false;
}

inline bool GroupMgr::remove(GroupId id, const AIPtr& ai) {
	ScopedWriteLock scopedLock(_lock);
	const GroupsIter& i = _groups.find(id);
	if (i == _groups.end()) {
		return false;
	}
	Group& group = i->second;
	GroupMembersSetIter si;
	{
		ScopedReadLock lock(_groupLock);
		si = group.members.find(ai);
		if (si == group.members.end()) {
			return false;
		}
	}
	{
		ScopedWriteLock lock(_groupLock);
		group.members.erase(si);
		if (group.members.empty()) {
			_groups.erase(i);
		} else if (group.leader == ai) {
			group.leader = *group.members.begin();
		}
	}

	auto range = _groupMembers.equal_range(ai);
	for (auto it = range.first; it != range.second; ++it) {
		if (it->second == id) {
			_groupMembers.erase(it);
			break;
		}
	}
	return true;
}

inline bool GroupMgr::removeFromAllGroups(const AIPtr& ai) {
	std::list<GroupId> groups;
	{
		ScopedReadLock scopedLock(_lock);
		auto range = _groupMembers.equal_range(ai);
		for (auto it = range.first; it != range.second; ++it) {
			groups.push_back(it->second);
		}
	}
	for (GroupId groupId : groups) {
		remove(groupId, ai);
	}
	return true;
}

inline AIPtr GroupMgr::getLeader(GroupId id) const {
	ScopedReadLock scopedLock(_lock);
	const GroupsConstIter& i = _groups.find(id);
	if (i == _groups.end()) {
		return AIPtr();
	}

	ScopedReadLock lock(_groupLock);
	return i->second.leader;
}

inline glm::vec3 GroupMgr::getPosition(GroupId id) const {
	ScopedReadLock scopedLock(_lock);
	const GroupsConstIter& i = _groups.find(id);
	if (i == _groups.end()) {
		return INFINITE;
	}

	ScopedReadLock lock(_groupLock);
	return i->second.position;
}

inline bool GroupMgr::isGroupLeader(GroupId id, const AIPtr& ai) const {
	ScopedReadLock scopedLock(_lock);
	const GroupsConstIter& i = _groups.find(id);
	if (i == _groups.end()) {
		return 0;
	}
	ScopedReadLock lock(_groupLock);
	return i->second.leader == ai;
}

inline int GroupMgr::getGroupSize(GroupId id) const {
	ScopedReadLock scopedLock(_lock);
	const GroupsConstIter& i = _groups.find(id);
	if (i == _groups.end()) {
		return 0;
	}
	ScopedReadLock lock(_groupLock);
	return static_cast<int>(std::distance(i->second.members.begin(), i->second.members.end()));
}

inline bool GroupMgr::isInAnyGroup(const AIPtr& ai) const {
	ScopedReadLock scopedLock(_lock);
	return _groupMembers.find(ai) != _groupMembers.end();
}

inline bool GroupMgr::isInGroup(GroupId id, const AIPtr& ai) const {
	ScopedReadLock scopedLock(_lock);
	auto range = _groupMembers.equal_range(ai);
	for (auto it = range.first; it != range.second; ++it) {
		if (it->second == id) {
			return true;
		}
	}
	return false;
}

}
