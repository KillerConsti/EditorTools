// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#pragma once

#include <QtCore\qabstractitemmodel.h>
#include <QtCore\qitemselectionmodel.h>

class TreeItem
{
public:
	explicit TreeItem(const QList<QVariant> &data, TreeItem *parentItem = nullptr);
	~TreeItem();

	void appendChild(TreeItem *child);

	TreeItem *child(int row);
	int childCount() const;
	int columnCount() const;
	QVariant data(int column) const;
	int row() const;
	TreeItem *parentItem();

private:
	QList<TreeItem *> m_childItems;
	QList<QVariant> m_itemData;
	TreeItem *m_parentItem;
};
//! [0]

