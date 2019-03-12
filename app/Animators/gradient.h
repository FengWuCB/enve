#ifndef GRADIENT_H
#define GRADIENT_H
#include <QGradientStops>
#include "Animators/complexanimator.h"
class ColorAnimator;
class PathBox;

class Gradient : public ComplexAnimator {
    Q_OBJECT
    friend class SelfRef;
public:
    bool SWT_isGradient() const { return true; }
    void prp_startTransform();
    void prp_setParentFrameShift(const int &shift,
                                 ComplexAnimator* parentAnimator = nullptr) {
        Q_UNUSED(shift);
        if(!parentAnimator) return;
        for(const auto &key : anim_mKeys) {
            parentAnimator->ca_updateDescendatKeyFrame(key.get());
        }
    }

    int prp_getFrameShift() const {
        return 0;
    }

    int prp_getParentFrameShift() const {
        return 0;
    }

    void writeProperty(QIODevice * const target) const;
    void readProperty(QIODevice *target);

    void swapColors(const int &id1, const int &id2);
    void removeColor(const qsptr<ColorAnimator> &color);
    void addColor(const QColor &color);
    void replaceColor(const int &id, const QColor &color);
    void addPath(PathBox * const path);
    void removePath(PathBox * const path);
    bool affectsPaths();

    //void finishTransform();

    void updateQGradientStops(const UpdateReason &reason);

    int getLoadId();
    void setLoadId(const int &id);

    void addColorToList(const QColor &color);
    QColor getCurrentColorAt(const int &id);
    int getColorCount();

    QColor getLastQGradientStopQColor();
    QColor getFirstQGradientStopQColor();

    QGradientStops getQGradientStops();
    void startColorIdTransform(const int &id);
    void addColorToList(const qsptr<ColorAnimator> &newColorAnimator);
    ColorAnimator *getColorAnimatorAt(const int &id);
    void removeColor(const int &id);

    bool isEmpty() const;

    QGradientStops getQGradientStopsAtAbsFrame(const qreal &absFrame);
signals:
    void resetGradientWidgetColorIdIfEquals(Gradient *, int);
protected:
    Gradient();
    Gradient(const QColor &color1, const QColor &color2);
private:
    int mLoadId = -1;
    QGradientStops mQGradientStops;
    QList<qsptr<ColorAnimator>> mColors;
    QList<qptr<PathBox>> mAffectedPaths;
    qptr<ColorAnimator> mCurrentColor;
};

#endif // GRADIENT_H
