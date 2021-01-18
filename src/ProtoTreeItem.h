#pragma once

#include <QList>
#include <QVariant>
#include <QVector>

class ItemRoleData
{
public:
  ItemRoleData() : role(-1) {}
  ItemRoleData(int r, const QVariant& v) : role(r), value(v) {}
  int role;
  QVariant value;
  bool operator==(const ItemRoleData& other) const { return role == other.role && value == other.value; }
};

class ProtoTreeItem
{
public:
    ProtoTreeItem(const QVector<QVariant> &data, ProtoTreeItem *parent = nullptr);
    ~ProtoTreeItem();

    void copyFrom(const ProtoTreeItem& item);

    ProtoTreeItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column, int role) const;
    bool insertChildren(int position, int count, int columns);
    bool insertChildren(int position, QList<ProtoTreeItem*> items);
    bool insertColumns(int position, int columns);
    ProtoTreeItem *parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    QList<ProtoTreeItem*> takeChildren(int position, int count);
    bool setData(int column, const QVariant &value, int role = Qt::DisplayRole);
    void setFlag(Qt::ItemFlags flags);
    Qt::ItemFlags getFlag();
private:
    QList<ProtoTreeItem*> childItems;
    QVector<QVariant> itemData;
    QVector<QVector<ItemRoleData>> values;
    ProtoTreeItem *parentItem;
    Qt::ItemFlags m_flags;
};
