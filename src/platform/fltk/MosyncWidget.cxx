// This file is part of SmallBASIC
//
// Copyright(C) 2001-2013 Chris Warren-Smith.
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
//

#include <fltk/Cursor.h>
#include <fltk/Font.h>
#include <fltk/Image.h>
#include <fltk/Monitor.h>
#include <fltk/draw.h>
#include <fltk/events.h>
#include <fltk/group.h>
#include <fltk/layout.h>
#include <fltk/run.h>

#include <time.h>
#include "MosyncWidget.h"
#include "platform/mosync/ansiwidget.h"

using namespace fltk;

#define SIZE_LIMIT 4

MosyncWidget *widget;
Canvas *drawTarget;
bool mouseActive;

int get_text_width(char *s) {
  return fltk::getwidth(s);
}

//
// Canvas
//
Canvas::Canvas(int size) :
  _img(NULL), 
  _clip(NULL),
  _rgb(0),
  _size(size),
  _style(0) {
}

Canvas::~Canvas() {
  if (_img) {
    _img->destroy();
  }
  delete _img;
  delete _clip;
}

void Canvas::beginDraw() {
  _img->make_current();
  setcolor(_rgb);
  setFont();
  if (_clip) {
    push_clip(*_clip);
  }
}

void Canvas::clear(int bg) {
  setcolor(bg);
  fillrect(0, 0, _img->w(), _img->h());
}

void Canvas::create(int w, int h, int bg) {
  GSave gsave;
  _img = new Image(w, h);
  _img->make_current();
  clear(bg);
}

void Canvas::draw(int w, int h) {
  push_clip(Rectangle(w, h));
  _img->draw(0, 0);
  pop_clip();
}

void Canvas::endDraw() {
  if (_clip) {
    pop_clip();
  }
}

void Canvas::resize(int w, int h) {
  if (_img) {
    int W = _img->w();
    int H = _img->h();
    if (w > W) {
      W = w;
    }
    if (h > H) {
    H = h;
    }
    Image *old = _img;
    GSave gsave;
    _img = new Image(W, H);
    _img->make_current();
    setcolor(widget->getBackgroundColor());
    fillrect(0, 0, W, H);
    old->draw(Rectangle(old->w(), old->h()));
    old->destroy();
    delete old;
  }
}

void Canvas::setClip(int x, int y, int w, int h) {
  delete _clip;
  _clip = new Rectangle(x, y, w, h);
}

void Canvas::setFont() {
  fltk::Font *font = fltk::COURIER;
  if (_size && _style) {
    if (_style && FONT_STYLE_BOLD) {
      font = font->bold();
    }
    if (_style && FONT_STYLE_ITALIC) {
      font = font->italic();
    }
  }
  setfont(font, _size);
}

//
// MosyncWidget
//
MosyncWidget::MosyncWidget(int x, int y, int w, int h, int defsize) :
  Widget(x, y, w, h, 0), 
  _ansiWidget(NULL),
  _screen(NULL),
  _resized(false),
  _defsize(defsize) {
  widget = this;
}

MosyncWidget::~MosyncWidget() {
  delete _ansiWidget;
  delete _screen;
}

void MosyncWidget::layout() {
  // deferred construction until FLTK is ready
  if (!_screen) {
    _screen = new Canvas(_defsize);
    _screen->create(w(), h(), BLACK);
    drawTarget = _screen;
  }
  if (!_ansiWidget) {
    _ansiWidget = new AnsiWidget(this, w(), h());
    _ansiWidget->construct();
    _ansiWidget->setTextColor(DEFAULT_COLOR, 0);
    _ansiWidget->setFontSize(_defsize);
  }
  if (layout_damage() & LAYOUT_WH) {
    // can't use GSave here in X
    _resized = true;
  }
  Widget::layout();
}

void MosyncWidget::draw() {
  // ensure this widget has lowest z-order
  int siblings = parent()->children();
  for (int n = 0; n < siblings; n++) {
    Widget *w = parent()->child(n);
    if (w != this) {
      w->redraw();
    }
  }

  if (_resized) {
    // resize the backing screens
    _screen->resize(w(), h());
    _resized = false;
  }

  if (_screen->_img) {
    _screen->draw(w(), h());
  } else {
    setcolor(getBackgroundColor());
    fillrect(Rectangle(w(), h()));
  }
}

void MosyncWidget::flush(bool force) {
  _ansiWidget->flush(force);
}

void MosyncWidget::reset() {
  _ansiWidget->setTextColor(DEFAULT_COLOR, 0);
  _ansiWidget->setFontSize(_defsize);
  _ansiWidget->reset();
}

int MosyncWidget::handle(int e) {
  MAEvent event;

  switch (e) {
  case FOCUS:
    return 1; 

  case PUSH:
    event.point.x = fltk::event_x();
    event.point.y = fltk::event_y();
    mouseActive = _ansiWidget->pointerTouchEvent(event);
    return mouseActive;

  case DRAG:
  case MOVE:
    event.point.x = fltk::event_x();
    event.point.y = fltk::event_y();
    if (mouseActive && _ansiWidget->pointerMoveEvent(event)) {
      Widget::cursor(fltk::CURSOR_HAND);
      return 1;
    }
    break;

  case RELEASE:
    mouseActive = false;
    Widget::cursor(fltk::CURSOR_DEFAULT);
    event.point.x = fltk::event_x();
    event.point.y = fltk::event_y();
    _ansiWidget->pointerReleaseEvent(event);
    break;
  }

  return Widget::handle(e);
}

void MosyncWidget::buttonClicked(const char *action) {
  
}

void MosyncWidget::clearScreen() {
  _ansiWidget->clearScreen();
}

void MosyncWidget::print(const char *str) {
  _ansiWidget->print(str);
}

void MosyncWidget::drawLine(int x1, int y1, int x2, int y2) {
  _ansiWidget->drawLine(x1, y1, x2, y2);
}

void MosyncWidget::drawRectFilled(int x1, int y1, int x2, int y2) {
  _ansiWidget->drawRectFilled(x1, y1, x2, y2);
}

void MosyncWidget::drawRect(int x1, int y1, int x2, int y2) {
  _ansiWidget->drawRect(x1, y1, x2, y2);
}

void MosyncWidget::drawImage(fltk::Image *img, int x, int y, int sx, int sy, int w, int h) {
  _ansiWidget->drawImage((MAHandle) img, x, y, sx, sy, w, h);
}

void MosyncWidget::saveImage(const char *fn, int x, int y, int w, int h) {
  // TODO
}

void MosyncWidget::setTextColor(long fg, long bg) {
  _ansiWidget->setTextColor(fg, bg);
}

void MosyncWidget::setColor(long color) {
  _ansiWidget->setColor(color);
}

int MosyncWidget::getX() {
  return _ansiWidget->getX();
}

int MosyncWidget::getY() {
  return _ansiWidget->getY();
}

void MosyncWidget::setPixel(int x, int y, int c) {
  _ansiWidget->setPixel(x, y, c);
}

int MosyncWidget::getPixel(int x, int y) {
  return _ansiWidget->getPixel(x, y);
}

void MosyncWidget::setXY(int x, int y) {
  _ansiWidget->setXY(x, y);
}

int MosyncWidget::textHeight(void) {
  return _ansiWidget->textHeight();
}

void MosyncWidget::setFontSize(float i) {
  _ansiWidget->setFontSize(i);
}

int MosyncWidget::getFontSize() {
  return _ansiWidget->getFontSize();
}

void MosyncWidget::resize(int x, int y, int w, int h) {
  _ansiWidget->resize(w, h);
  Widget::resize(x, y, w, h);
}

int MosyncWidget::getBackgroundColor() {
  return _ansiWidget->getBackgroundColor();
}

//
// maapi implementation
//
int maFontDelete(MAHandle maHandle) {
  return RES_FONT_OK;
}

int maSetColor(int rgb) {
  if (drawTarget) {
    drawTarget->_rgb = rgb;
  }
  return rgb;
}

void maSetClipRect(int left, int top, int width, int height) {
  if (drawTarget) {
    drawTarget->setClip(left, top, width, height);
  }
}

void maPlot(int posX, int posY) {
  if (drawTarget) {
    GSave gsave;
    drawTarget->beginDraw();
    drawpoint(posX, posY);
    drawTarget->endDraw();
  }
}

void maLine(int startX, int startY, int endX, int endY) {
  if (drawTarget) {
    GSave gsave;
    drawTarget->beginDraw();
    drawline(startX, startY, endX, endY);
    drawTarget->endDraw();
  }
}

void maFillRect(int left, int top, int width, int height) {
  if (drawTarget) {
    GSave gsave;
    drawTarget->beginDraw();
    fillrect(left, top, width, height);
    drawTarget->endDraw();
  }
}

void maDrawText(int left, int top, const char *str) {
  if (drawTarget) {
    GSave gsave;
    drawTarget->beginDraw();
    drawtext(str, left, top + (int)getascent());
    drawTarget->endDraw();
  }
}

void maUpdateScreen(void) {
  fltk::redraw();
}

void maResetBacklight(void) {
}

MAExtent maGetTextSize(const char *str) {
  if (drawTarget) {
    drawTarget->setFont();
  }
  short width = (int)getwidth(str);
  short height = (int)(getascent() + getdescent());
  return (MAExtent)((width << 16) + height);
}

MAExtent maGetScrSize(void) {
  short width = widget->w();
  short height = widget->h();
  return (MAExtent)((width << 16) && height);
}

MAHandle maFontLoadDefault(int type, int style, int size) {
  MAHandle result;
  if (drawTarget) {
    drawTarget->_style = style;
    result = (MAHandle)drawTarget;
  } else {
    result = NULL;
  }
  return result;
}

MAHandle maFontSetCurrent(MAHandle maHandle) {
  return maHandle;
}

void maDrawImageRegion(MAHandle maHandle, const MARect *srcRect, 
                       const MAPoint2d *dstPoint, int transformMode) {
  Canvas *canvas = (Canvas *)maHandle;
  if (drawTarget && canvas->_img && canvas != drawTarget) {
    Rectangle from = Rectangle(srcRect->left, srcRect->top, srcRect->width, srcRect->height);
    Rectangle to = Rectangle(dstPoint->x, dstPoint->y, srcRect->width, srcRect->height);
    GSave gsave;
    drawTarget->beginDraw();
    canvas->_img->draw(from, to);
    drawTarget->endDraw();
  }
}

int maCreateDrawableImage(MAHandle maHandle, int width, int height) {
  int result = RES_OK;
  const fltk::Monitor &monitor = fltk::Monitor::all();
  if (height > monitor.h() * SIZE_LIMIT) {
    result -= 1;
  } else {
    Canvas *canvas = (Canvas *)maHandle;
    canvas->create(width, height, widget->getBackgroundColor());
  }
  return result;
}

MAHandle maCreatePlaceholder(void) {
  MAHandle maHandle = (MAHandle) new Canvas(widget->getDefaultSize());
  return maHandle;
}

void maDestroyPlaceholder(MAHandle maHandle) {
  Canvas *holder = (Canvas *)maHandle;
  delete holder;
}

void maGetImageData(MAHandle maHandle, void *dst, const MARect *srcRect, int scanlength) {

}

MAHandle maSetDrawTarget(MAHandle maHandle) {
  if (maHandle == (MAHandle) HANDLE_SCREEN) {
    drawTarget = widget->getScreen();
  } else {
    drawTarget = (Canvas *)maHandle;
  }
  return (MAHandle) drawTarget;
}

int maGetMilliSecondCount(void) {
  return clock();
}

int maShowVirtualKeyboard(void) {
  return 0;
}

void maOptionsBox(const wchar *title, const wchar *destructiveButtonTitle,
                  const wchar *cancelButtonTitle, const void *otherButtonTitles,
                  int otherButtonTitlesSize) {
}

int maGetEvent(MAEvent *event) {
  int result = 0;
  if (check()) {
    switch (fltk::event()) {
    case PUSH:
      event->type = EVENT_TYPE_POINTER_PRESSED;
      result = 1;
      break;
    case DRAG:
      event->type = EVENT_TYPE_POINTER_DRAGGED;
      result = 1;
      break;
    case RELEASE:
      event->type = EVENT_TYPE_POINTER_RELEASED;
      result = 1;
      break;
    }
  }
  return result;
}

void maWait(int timeout) {
  fltk::wait(timeout);
}

void maAlert(const char *title, const char *message, const char *button1,
             const char *button2, const char *button3) {
}
