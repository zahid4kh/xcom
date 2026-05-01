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

#include <QWebEnginePage>

class MainWindow;

class XComPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit XComPage(MainWindow* mainWindow, QWebEngineProfile* profile,
                      QObject* parent = nullptr);

protected:
    QWebEnginePage* createWindow(WebWindowType type) override;
    bool acceptNavigationRequest(const QUrl& url, NavigationType type,
                                 bool isMainFrame) override;

private:
    MainWindow* m_mainWindow;
    bool m_googleAuthWarningShown = false;

    bool openGoogleAuthExternally(const QUrl& url);
    static bool isGoogleAuthUrl(const QUrl& url);
    static bool isXDomain(const QUrl& url);
    static bool isXUrl(const QUrl& url);
};
