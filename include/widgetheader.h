// Copyright 2026 Zahid Khalilov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <QWidget>

class WidgetHeader : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetHeader(QWidget *parent = nullptr);
    QSize sizeHint() const override { return {400, 40}; }

signals:
    void exitRequested();
    void hideRequested();
    void reloadRequested();

protected:
    void mousePressEvent(QMouseEvent *) override;
};
