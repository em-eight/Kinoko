#include "ItemInventory.hh"

namespace Item {

ItemInventory::ItemInventory() = default;

ItemInventory::~ItemInventory() = default;

void ItemInventory::setItem(ItemId id) {
    constexpr int MUSHROOM_COUNT = 3;

    m_currentId = id;
    m_currentCount = MUSHROOM_COUNT;
}

void ItemInventory::useItem(int count) {
    m_currentCount -= count;
    if (m_currentCount > 0) {
        return;
    }

    m_currentId = ItemId::NONE;
    m_currentCount = 0;
}

ItemId ItemInventory::id() const {
    return m_currentId;
}

} // namespace Item
