#include <escher/expression_layout_field.h>
#include <apps/i18n.h>
#include <escher/clipboard.h>
#include <escher/text_field.h>
#include <poincare/expression.h>
#include <poincare/expression_layout_cursor.h>
#include <poincare/src/layout/matrix_layout.h>
#include <assert.h>
#include <string.h>

ExpressionLayoutField::ExpressionLayoutField(Responder * parentResponder, Poincare::ExpressionLayout * expressionLayout, ExpressionLayoutFieldDelegate * delegate) :
  ScrollableView(parentResponder, &m_contentView, this),
  m_contentView(expressionLayout),
  m_delegate(delegate)
{
}

bool ExpressionLayoutField::isEditing() const {
  return m_contentView.isEditing();
}

void ExpressionLayoutField::setEditing(bool isEditing) {
  m_contentView.setEditing(isEditing);
}

void ExpressionLayoutField::clearLayout() {
  m_contentView.clearLayout();
}

void ExpressionLayoutField::scrollToCursor() {
  scrollToBaselinedRect(m_contentView.cursorRect(), m_contentView.cursor()->baseline());
}

Toolbox * ExpressionLayoutField::toolbox() {
  if (m_delegate) {
    return m_delegate->toolboxForExpressionLayoutField(this);
  }
  return nullptr;
}

bool ExpressionLayoutField::handleEvent(Ion::Events::Event event) {
  bool didHandleEvent = false;
  bool shouldRecomputeLayout = m_contentView.cursor()->showEmptyLayoutIfNeeded();
  bool moveEventChangedLayout = false;
  if (privateHandleMoveEvent(event, &moveEventChangedLayout)) {
    if (!isEditing()) {
      setEditing(true);
    }
    shouldRecomputeLayout = shouldRecomputeLayout || moveEventChangedLayout;
    didHandleEvent = true;
  } else if (privateHandleEvent(event)) {
    shouldRecomputeLayout = true;
    didHandleEvent = true;
  }
  if (didHandleEvent) {
    shouldRecomputeLayout = m_contentView.cursor()->hideEmptyLayoutIfNeeded() || shouldRecomputeLayout;
    if (!shouldRecomputeLayout) {
      m_contentView.cursorPositionChanged();
      scrollToCursor();
    } else {
      reload();
    }
    return true;
  }
  m_contentView.cursor()->hideEmptyLayoutIfNeeded();
  return false;
}

bool ExpressionLayoutField::expressionLayoutFieldShouldFinishEditing(Ion::Events::Event event) {
  return m_delegate->expressionLayoutFieldShouldFinishEditing(this, event);
}

KDSize ExpressionLayoutField::minimalSizeForOptimalDisplay() const {
  KDSize contentViewSize = m_contentView.minimalSizeForOptimalDisplay();
  return KDSize(contentViewSize.width(), contentViewSize.height());
}

bool ExpressionLayoutField::privateHandleMoveEvent(Ion::Events::Event event, bool * shouldRecomputeLayout) {
  Poincare::ExpressionLayoutCursor result;
  if (event == Ion::Events::Left) {
    result = m_contentView.cursor()->cursorOnLeft(shouldRecomputeLayout);
  } else if (event == Ion::Events::Right) {
    result =  m_contentView.cursor()->cursorOnRight(shouldRecomputeLayout);
  } else if (event == Ion::Events::Up) {
    result = m_contentView.cursor()->cursorAbove(shouldRecomputeLayout);
  } else if (event == Ion::Events::Down) {
    result = m_contentView.cursor()->cursorUnder(shouldRecomputeLayout);
  } else if (event == Ion::Events::ShiftLeft) {
    *shouldRecomputeLayout = true;
    if (m_contentView.cursor()->pointedExpressionLayout()->removeGreySquaresFromAllMatrixAncestors()) {
      *shouldRecomputeLayout = true;
    }
    result.setPointedExpressionLayout(expressionLayout());
    result.setPosition(Poincare::ExpressionLayoutCursor::Position::Left);
  } else if (event == Ion::Events::ShiftRight) {
    if (m_contentView.cursor()->pointedExpressionLayout()->removeGreySquaresFromAllMatrixAncestors()) {
      *shouldRecomputeLayout = true;
    }
    result.setPointedExpressionLayout(expressionLayout());
    result.setPosition(Poincare::ExpressionLayoutCursor::Position::Right);
  }
  if (result.isDefined()) {
    m_contentView.setCursor(result);
    return true;
  }
  return false;
}

bool ExpressionLayoutField::privateHandleEvent(Ion::Events::Event event) {
  if (m_delegate && m_delegate->expressionLayoutFieldDidReceiveEvent(this, event)) {
    return true;
  }
  if (Responder::handleEvent(event)) {
    /* The only event Responder handles is 'Toolbox' displaying. In that case,
     * the ExpressionLayoutField is forced into editing mode. */
    if (!isEditing()) {
      setEditing(true);
    }
    return true;
  }
  if (isEditing() && expressionLayoutFieldShouldFinishEditing(event)) {
    setEditing(false);
    if (m_delegate->expressionLayoutFieldDidFinishEditing(this, expressionLayout(), event)) {
      clearLayout();
    }
    return true;
  }
  if ((event == Ion::Events::OK || event == Ion::Events::EXE) && !isEditing()) {
    setEditing(true);
    m_contentView.cursor()->setPointedExpressionLayout(expressionLayout());
    m_contentView.cursor()->setPosition(Poincare::ExpressionLayoutCursor::Position::Right);
    return true;
  }
  if (event == Ion::Events::Back && isEditing()) {
    clearLayout();
    setEditing(false);
    m_delegate->expressionLayoutFieldDidAbortEditing(this);
    return true;
  }
  if (event.hasText() || event == Ion::Events::Paste || event == Ion::Events::Backspace) {
    if (!isEditing()) {
      setEditing(true);
    }
    if (event.hasText()) {
      handleEventWithText(event.text());
    } else if (event == Ion::Events::Paste) {
      handleEventWithText(Clipboard::sharedClipboard()->storedText(), false, true);
    } else {
      assert(event == Ion::Events::Backspace);
      m_contentView.cursor()->performBackspace();
    }
    return true;
  }
  if (event == Ion::Events::Clear && isEditing()) {
    clearLayout();
    return true;
  }
  return false;
}

void ExpressionLayoutField::reload() {
  KDSize previousSize = minimalSizeForOptimalDisplay();
  expressionLayout()->invalidAllSizesPositionsAndBaselines();
  KDSize newSize = minimalSizeForOptimalDisplay();
  if (m_delegate && previousSize.height() != newSize.height()) {
    m_delegate->expressionLayoutFieldDidChangeSize(this);
  }
  m_contentView.cursorPositionChanged();
  scrollToCursor();
  markRectAsDirty(bounds());
}

bool ExpressionLayoutField::hasText() const {
  return expressionLayout()->hasText();
}

int ExpressionLayoutField::writeTextInBuffer(char * buffer, int bufferLength) {
  return expressionLayout()->writeTextInBuffer(buffer, bufferLength);
}

bool ExpressionLayoutField::handleEventWithText(const char * text, bool indentation, bool forceCursorRightOfText) {
  if (text[0] == 0) {
    // The text is empty
    return true;
  }

  int currentNumberOfLayouts = m_contentView.expressionView()->numberOfLayouts();
  if (currentNumberOfLayouts >= k_maxNumberOfLayouts - 6) {
    /* We add -6 because in some cases (Ion::Events::Division,
     * Ion::Events::Exp...) we let the layout cursor handle the layout insertion
     * and these events may add at most 6 layouts (e.g *10^ø). */
    return true;
  }

  // Handle special cases
  if (strcmp(text, Ion::Events::Division.text()) == 0) {
    m_contentView.cursor()->addFractionLayoutAndCollapseSiblings();
  } else if (strcmp(text, Ion::Events::Exp.text()) == 0) {
    m_contentView.cursor()->addEmptyExponentialLayout();
  } else if (strcmp(text, Ion::Events::Power.text()) == 0) {
    m_contentView.cursor()->addEmptyPowerLayout();
  } else if (strcmp(text, Ion::Events::Sqrt.text()) == 0) {
    m_contentView.cursor()->addEmptySquareRootLayout();
  } else if (strcmp(text, Ion::Events::Square.text()) == 0) {
    m_contentView.cursor()->addEmptySquarePowerLayout();
  } else if (strcmp(text, Ion::Events::EE.text()) == 0) {
    m_contentView.cursor()->addEmptyTenPowerLayout();
  } else if ((strcmp(text, "[") == 0) || (strcmp(text, "]") == 0)) {
    m_contentView.cursor()->addEmptyMatrixLayout();
  } else {
    Poincare::Expression * resultExpression = Poincare::Expression::parse(text);
    if (resultExpression == nullptr) {
      m_contentView.cursor()->insertText(text);
      return true;
    }
    Poincare::ExpressionLayout * resultLayout = resultExpression->createLayout(Poincare::Preferences::sharedPreferences()->displayMode(), Poincare::Preferences::sharedPreferences()->numberOfSignificantDigits());
    delete resultExpression;
    if (currentNumberOfLayouts + resultLayout->numberOfDescendants(true) >= k_maxNumberOfLayouts) {
      delete resultLayout;
      return true;
    }
    // Find the pointed layout.
    Poincare::ExpressionLayout * pointedLayout = nullptr;
    if (strcmp(text, I18n::translate(I18n::Message::RandomCommandWithArg)) == 0) {
      /* Special case: if the text is "random()", the cursor should not be set
       * inside the parentheses. */
      pointedLayout = resultLayout;
    } else if (resultLayout->isHorizontal()) {
      /* If the layout is horizontal, pick the first open parenthesis. For now,
       * all horizontal layouts in MathToolbox have parentheses. */
      for (int i = 0; i < resultLayout->numberOfChildren(); i++) {
        if (resultLayout->editableChild(i)->isLeftParenthesis()) {
          pointedLayout = resultLayout->editableChild(i);
          break;
        }
      }
    }
    /* Insert the layout. If pointedLayout is nullptr, the cursor will be on the
     * right of the inserted layout. */
    insertLayoutAtCursor(resultLayout, pointedLayout, forceCursorRightOfText);
  }
  return true;
}

Poincare::ExpressionLayout * ExpressionLayoutField::expressionLayout() const {
  return m_contentView.expressionView()->expressionLayout();
}

char ExpressionLayoutField::XNTChar() {
  return m_contentView.cursor()->pointedExpressionLayout()->XNTChar();
}

void ExpressionLayoutField::setBackgroundColor(KDColor c) {
  ScrollableView::setBackgroundColor(c);
  m_contentView.setBackgroundColor(c);
}

void ExpressionLayoutField::scrollRightOfLayout(Poincare::ExpressionLayout * layout) {
  KDRect layoutRect(layout->absoluteOrigin().translatedBy(m_contentView.expressionView()->drawingOrigin()), layout->size());
  scrollToBaselinedRect(layoutRect, layout->baseline());
}

void ExpressionLayoutField::scrollToBaselinedRect(KDRect rect, KDCoordinate baseline) {
  scrollToContentRect(rect, true);
  // Show the rect area around its baseline
  KDCoordinate underBaseline = rect.height() - baseline;
  KDCoordinate minAroundBaseline = min(baseline, underBaseline);
  minAroundBaseline = min(minAroundBaseline, bounds().height() / 2);
  KDRect balancedRect(rect.x(), rect.y() + baseline - minAroundBaseline, rect.width(), 2 * minAroundBaseline);
  scrollToContentRect(balancedRect, true);
}

void ExpressionLayoutField::insertLayoutAtCursor(Poincare::ExpressionLayout * layout, Poincare::ExpressionLayout * pointedLayout, bool forceCursorRightOfLayout) {
  if (layout == nullptr) {
    return;
  }
  m_contentView.cursor()->showEmptyLayoutIfNeeded();
  bool layoutWillBeMerged = layout->isHorizontal();
  Poincare::ExpressionLayout * lastMergedLayoutChild = layoutWillBeMerged ? layout->editableChild(layout->numberOfChildren()-1) : nullptr;
  m_contentView.cursor()->addLayoutAndMoveCursor(layout);
  if (!forceCursorRightOfLayout) {
    if (pointedLayout != nullptr && (pointedLayout != layout || !layoutWillBeMerged)) {
      m_contentView.cursor()->setPointedExpressionLayout(pointedLayout);
      m_contentView.cursor()->setPosition(Poincare::ExpressionLayoutCursor::Position::Right);
    } else if (!layoutWillBeMerged) {
      m_contentView.cursor()->setPointedExpressionLayout(layout->layoutToPointWhenInserting());
      m_contentView.cursor()->setPosition(Poincare::ExpressionLayoutCursor::Position::Right);
    }
  } else if (!layoutWillBeMerged) {
    m_contentView.cursor()->setPointedExpressionLayout(layout);
    m_contentView.cursor()->setPosition(Poincare::ExpressionLayoutCursor::Position::Right);
  }
  m_contentView.cursor()->pointedExpressionLayout()->addGreySquaresToAllMatrixAncestors();
  m_contentView.cursor()->hideEmptyLayoutIfNeeded();
  reload();
  if (!layoutWillBeMerged) {
    scrollRightOfLayout(layout);
  } else {
    assert(lastMergedLayoutChild != nullptr);
    scrollRightOfLayout(lastMergedLayoutChild);
  }
  scrollToCursor();
}
