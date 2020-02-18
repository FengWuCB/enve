// enve - 2D animations software
// Copyright (C) 2016-2020 Maurycy Liebner

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "Private/document.h"
#include "FileCacheHandlers/filecachehandler.h"
#include "canvas.h"
#include "simpletask.h"

Document* Document::sInstance = nullptr;

bool Document::FileCompare::operator()(const FileHandler &f1,
                                       const FileHandler &f2) {
    return f1->getFilePath() < f2->getFilePath();
}

Document::Document(TaskScheduler& taskScheduler) {
    Q_ASSERT(!sInstance);
    sInstance = this;
    connect(&taskScheduler, &TaskScheduler::finishedAllQuedTasks,
            this, &Document::updateScenes);
}

void Document::updateScenes() {
    SimpleTask::sProcessAll();
    TaskScheduler::sInstance->queTasks();

    for(const auto& scene : fVisibleScenes) {
        emit scene.first->requestUpdate();
    }
}

void Document::actionFinished() {
    updateScenes();
    for(const auto& scene : fVisibleScenes) {
        const auto newUndoRedo = scene.first->newUndoRedoSet();
        if(newUndoRedo) emit documentChanged();
    }
}

void Document::replaceClipboard(const stdsptr<Clipboard> &container) {
    fClipboardContainer = container;
}

Clipboard *Document::getClipboard(const ClipboardType type) const {
    if(!fClipboardContainer) return nullptr;
    if(type == fClipboardContainer->getType())
        return fClipboardContainer.get();
    return nullptr;
}

DynamicPropsClipboard* Document::getDynamicPropsClipboard() const {
    auto contT = getClipboard(ClipboardType::dynamicProperties);
    return static_cast<DynamicPropsClipboard*>(contT);
}

PropertyClipboard* Document::getPropertyClipboard() const {
    auto contT = getClipboard(ClipboardType::property);
    return static_cast<PropertyClipboard*>(contT);
}

KeysClipboard* Document::getKeysClipboard() const {
    auto contT = getClipboard(ClipboardType::keys);
    return static_cast<KeysClipboard*>(contT);
}

BoxesClipboard* Document::getBoxesClipboard() const {
    auto contT = getClipboard(ClipboardType::boxes);
    return static_cast<BoxesClipboard*>(contT);
}

SmartPathClipboard* Document::getSmartPathClipboard() const {
    auto contT = getClipboard(ClipboardType::smartPath);
    return static_cast<SmartPathClipboard*>(contT);
}

void Document::setPath(const QString &path) {
    fEvFile = path;
    emit evFilePathChanged(fEvFile);
}

void Document::setCanvasMode(const CanvasMode mode) {
    fCanvasMode = mode;
    emit canvasModeSet(mode);
    actionFinished();
}

Canvas *Document::createNewScene() {
    const auto newScene = enve::make_shared<Canvas>(*this);
    fScenes.append(newScene);
    SWT_addChild(newScene.get());
    emit sceneCreated(newScene.get());
    return newScene.get();
}

bool Document::removeScene(const qsptr<Canvas>& scene) {
    const int id = fScenes.indexOf(scene);
    return removeScene(id);
}

bool Document::removeScene(const int id) {
    if(id < 0 || id >= fScenes.count()) return false;
    const auto scene = fScenes.takeAt(id);
    SWT_removeChild(scene.data());
    emit sceneRemoved(scene.data());
    emit sceneRemoved(id);
    return true;
}

void Document::addVisibleScene(Canvas * const scene) {
    fVisibleScenes[scene]++;
    updateScenes();
}

bool Document::removeVisibleScene(Canvas * const scene) {
    const auto it = fVisibleScenes.find(scene);
    if(it == fVisibleScenes.end()) return false;
    if(it->second == 1) fVisibleScenes.erase(it);
    else it->second--;
    return true;
}

void Document::setActiveScene(Canvas * const scene) {
    if(scene == fActiveScene) return;
    if(fActiveScene) {
        disconnect(fActiveScene, nullptr, this, nullptr);
    }
    fActiveScene = scene;
    if(fActiveScene) {
        connect(fActiveScene, &Canvas::currentBoxChanged,
                this, &Document::currentBoxChanged);
        connect(fActiveScene, &Canvas::selectedPaintSettingsChanged,
                this, &Document::selectedPaintSettingsChanged);
        connect(fActiveScene, &Canvas::destroyed,
                this, &Document::clearActiveScene);
        emit currentBoxChanged(fActiveScene->getCurrentBox());
        emit selectedPaintSettingsChanged();
    }
    emit activeSceneSet(scene);
}

void Document::clearActiveScene() {
    setActiveScene(nullptr);
}

int Document::getActiveSceneFrame() const {
    if(!fActiveScene) return 0;
    return fActiveScene->anim_getCurrentAbsFrame();
}

void Document::setActiveSceneFrame(const int frame) {
    if(!fActiveScene) return;
    if(fActiveScene->anim_getCurrentRelFrame() == frame) return;
    fActiveScene->anim_setAbsFrame(frame);
    emit activeSceneFrameSet(frame);
}

void Document::incActiveSceneFrame() {
    setActiveSceneFrame(getActiveSceneFrame() + 1);
}

void Document::decActiveSceneFrame() {
    setActiveSceneFrame(getActiveSceneFrame() - 1);
}

void Document::addBookmarkBrush(SimpleBrushWrapper * const brush) {
    if(!brush) return;
    removeBookmarkBrush(brush);
    fBrushes << brush;
    emit bookmarkBrushAdded(brush);
}

void Document::removeBookmarkBrush(SimpleBrushWrapper * const brush) {
    if(fBrushes.removeOne(brush))
        emit bookmarkBrushRemoved(brush);
}

void Document::addBookmarkColor(const QColor &color) {
    removeBookmarkColor(color);
    fColors << color;
    emit bookmarkColorAdded(color);
}

void Document::removeBookmarkColor(const QColor &color) {
    const auto rgba = color.rgba();
    for(const auto& iColor : fColors) {
        if(iColor.rgba() == rgba) {
            emit bookmarkColorRemoved(color);
            break;
        }
    }
}

void Document::setBrush(BrushContexedWrapper * const brush) {
    fBrush = brush->getSimpleBrush();
    if(fBrush) fBrush->setColor(fBrushColor);
    emit brushChanged(brush);
    emit brushSizeChanged(fBrush ? fBrush->getBrushSize() : 0.f);
    emit brushColorChanged(fBrush ? fBrush->getColor() : Qt::white);
}

void Document::setBrushColor(const QColor &color) {
    fBrushColor = color;
    if(fBrush) fBrush->setColor(fBrushColor);
    fSculptBrush.setColor(color);
    emit brushColorChanged(color);
}

void Document::incBrushRadius() {
    if(!fBrush) return;
    fBrush->incPaintBrushSize(0.3);
    emit brushSizeChanged(fBrush->getBrushSize());
}

void Document::decBrushRadius() {
    if(!fBrush) return;
    fBrush->decPaintBrushSize(0.3);
    emit brushSizeChanged(fBrush->getBrushSize());
}

void Document::setOnionDisabled(const bool disabled) {
    fOnionVisible = !disabled;
    actionFinished();
}

void Document::setSculptNodesHidden(const bool hidden) {
    fSculptNodesVisible = !hidden;
    actionFinished();
}

void Document::incSculptBrushRadius() {
    fSculptBrush.incExpRadius(0.3);
    emit sculptBrushSizeChanged(fSculptBrush.radius());
}

void Document::decSculptBrushRadius() {
    fSculptBrush.decExpRadius(0.3);
    emit sculptBrushSizeChanged(fSculptBrush.radius());
}

void Document::incSculptHardness() {
    const qreal oldHardness = fSculptBrush.hardness();
    fSculptBrush.setHardness(oldHardness + 0.1);
    emit sculptHardnessChanged(fSculptBrush.hardness());
}

void Document::decSculptHardness() {
    const qreal oldHardness = fSculptBrush.hardness();
    fSculptBrush.setHardness(oldHardness - 0.1);
    emit sculptHardnessChanged(fSculptBrush.hardness());
}

void Document::incSculptOpacity() {
    const qreal oldOpacity = fSculptBrush.opacity();
    fSculptBrush.setOpacity(oldOpacity + 0.1);
    emit sculptOpacityChanged(fSculptBrush.opacity());
}

void Document::decSculptOpacity() {
    const qreal oldOpacity = fSculptBrush.opacity();
    fSculptBrush.setOpacity(oldOpacity - 0.1);
    emit sculptOpacityChanged(fSculptBrush.opacity());
}

void Document::setSculptTarget(const SculptTarget target) {
    fSculptTarget = target;
    emit sculptTargetChanged(target);
}

void Document::setSculptMode(const SculptMode mode) {
    fSculptMode = mode;
    emit sculptModeChanged(mode);
}

void Document::clear() {
    setPath("");
    const int nScenes = fScenes.count();
    for(int i = 0; i < nScenes; i++) removeScene(0);
    replaceClipboard(nullptr);
    for(const auto brush : fBrushes) {
        removeBookmarkBrush(brush);
    }
    fBrushes.clear();
    for(const auto& color : fColors) {
        removeBookmarkColor(color);
    }
    fColors.clear();
}

void Document::SWT_setupAbstraction(SWT_Abstraction * const abstraction,
                                    const UpdateFuncs &updateFuncs,
                                    const int visiblePartWidgetId) {
    for(const auto& scene : fScenes) {
        auto abs = scene->SWT_abstractionForWidget(updateFuncs,
                                                   visiblePartWidgetId);
        abstraction->addChildAbstraction(abs->ref<SWT_Abstraction>());
    }
}
