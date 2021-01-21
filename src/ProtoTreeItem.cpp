#include <QStringList>

#include "ProtoTreeItem.h"

ProtoTreeItem::ProtoTreeItem(const QVector<QVariant>& data, ProtoTreeItem* parent) {
  parentItem = parent;
  itemData = data;
  values.resize(itemData.size());
}

ProtoTreeItem::~ProtoTreeItem() {
  qDeleteAll(childItems);
}

void ProtoTreeItem::copyFrom(const ProtoTreeItem& item) {
  itemData = item.itemData;
  values = item.values;
  m_flags = item.m_flags;
}

ProtoTreeItem* ProtoTreeItem::child(int number) {
  return childItems.value(number);
}

int ProtoTreeItem::childCount() const {
  return childItems.count();
}

int ProtoTreeItem::childNumber() const {
  if (parentItem) {
    return parentItem->childItems.indexOf(const_cast<ProtoTreeItem*>(this));
  }

  return 0;
}

QList<ProtoTreeItem*> ProtoTreeItem::takeChildren(int position, int count) {
  QList<ProtoTreeItem*> list;
  if (position < 0 || position + count > childItems.size()) {
    return list;
  }


  for (int row = 0; row < count; ++row) {
    list.append(childItems.takeAt(position));
  }

  return list;
}

int ProtoTreeItem::columnCount() const {
  return itemData.count();
}

QVariant ProtoTreeItem::data(int column, int role) const {
  switch (role) {
  case Qt::EditRole:
  case Qt::DisplayRole:
    if (column >= 0 && column < itemData.count())
      return itemData.at(column);
    break;
  default:
    if (column >= 0 && column < values.size()) {
      const QVector<ItemRoleData>& column_values = values.at(column);
      for (const auto& column_value : column_values) {
        if (column_value.role == role)
          return column_value.value;
      }
    }
  }
  return QVariant();
}

bool ProtoTreeItem::insertChildren(int position, int count, int columns) {
  if (position < 0 || position > childItems.size()) {
    return false;
  }

  for (int row = 0; row < count; ++row) {
    QVector<QVariant> data(columns);
    ProtoTreeItem* item = new ProtoTreeItem(data, this);
    childItems.insert(position, item);
  }

  return true;
}

bool ProtoTreeItem::insertChildren(int position, QList<ProtoTreeItem*> items) {
  if (position < 0 || position > childItems.size()) {
    return false;
  }

  auto it = items.rbegin();
  for (; it != items.rend(); ++it) {
    childItems.insert(position, *it);
  }

  return true;
}

bool ProtoTreeItem::insertColumns(int position, int columns) {
  if (position < 0 || position > itemData.size()) {
    return false;
  }

  for (int column = 0; column < columns; ++column) {
    itemData.insert(position, QVariant());
  }

  foreach(ProtoTreeItem * child, childItems) {
    child->insertColumns(position, columns);
  }

  return true;
}

ProtoTreeItem* ProtoTreeItem::parent() {
  return parentItem;
}

bool ProtoTreeItem::removeChildren(int position, int count) {
  if (position < 0 || position + count > childItems.size()) {
    return false;
  }

  for (int row = 0; row < count; ++row) {
    delete childItems.takeAt(position);
  }

  return true;
}

bool ProtoTreeItem::removeColumns(int position, int columns) {
  if (position < 0 || position + columns > itemData.size()) {
    return false;
  }

  for (int column = 0; column < columns; ++column) {
    itemData.remove(position);
  }

  foreach(ProtoTreeItem * child, childItems) {
    child->removeColumns(position, columns);
  }

  return true;
}

bool ProtoTreeItem::setData(int column, const QVariant& value, int role) {
  if (column < 0) {
    return false;
  }

  switch (role) {
  case Qt::EditRole:
  case Qt::DisplayRole: {
      if (itemData.count() <= column) {
        for (int i = itemData.count() - 1; i < column - 1; ++i) {
          itemData.append(QVariant());
        }
        itemData.append(value);
      } else if (itemData[column] != value) {
        itemData[column] = value;
      } else {
        return false; // value is unchanged
      }
    } break;
  default:
    if (column < values.count()) {
      bool found = false;
      const QVector<ItemRoleData> column_values = values.at(column);
      for (int i = 0; i < column_values.count(); ++i) {
        if (column_values.at(i).role == role) {
          if (column_values.at(i).value == value) {
            return false; // value is unchanged
          }
          values[column][i].value = value;
          found = true;
          break;
        }
      }
      if (!found) {
        values[column].append(ItemRoleData(role, value));
      }
    } else {
      return false;
    }
  }

  return true;
}

void ProtoTreeItem::setFlag(Qt::ItemFlags flags) {
  m_flags = flags;
}

Qt::ItemFlags ProtoTreeItem::getFlag() {
  return m_flags;
}

void ProtoTreeItem::getIndexOfMessage(std::list<int32_t>& rows) {
  if (parentItem) {
    parentItem->getIndexOfMessage(rows);

    if ((m_flags & Qt::ItemIsDragEnabled) != Qt::ItemIsDragEnabled)
      rows.emplace_back(childNumber());
  }
}
