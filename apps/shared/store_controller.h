#ifndef SHARED_STORE_CONTROLLER_H
#define SHARED_STORE_CONTROLLER_H

#include <escher.h>
#include "buffer_text_view_with_text_field.h"
#include "editable_cell_table_view_controller.h"
#include "double_pair_store.h"
#include "store_cell.h"
#include "store_context.h"
#include "store_parameter_controller.h"
#include "store_selectable_table_view.h"

namespace Shared {

class StoreController : public EditableCellTableViewController, public ButtonRowDelegate  {
public:
  StoreController(Responder * parentResponder, DoublePairStore * store, ButtonRowController * header);

  virtual StoreContext * storeContext() = 0;
  void displayFormulaInput();
  virtual void setFormulaLabel() = 0;
  virtual bool fillColumnWithFormula(Poincare::Expression * formula) = 0;

  // TextFieldDelegate
  bool textFieldShouldFinishEditing(TextField * textField, Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(TextField * textField, const char * text, Ion::Events::Event event) override;
  bool textFieldDidAbortEditing(TextField * textField) override;

  // TableViewDataSource
  int numberOfColumns() override;
  KDCoordinate columnWidth(int i) override;
  KDCoordinate cumulatedWidthFromIndex(int i) override;
  int indexFromCumulatedWidth(KDCoordinate offsetX) override;
  HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int i, int j) override;
  void willDisplayCellAtLocation(HighlightCell * cell, int i, int j) override;

  // ViewController
  const char * title() override;

  // Responder
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;

protected:
  static constexpr KDCoordinate k_cellWidth = 116;
  static constexpr KDCoordinate k_margin = 8;
  static constexpr KDCoordinate k_scrollBarMargin = Metric::CommonRightMargin;
  constexpr static int k_maxNumberOfEditableCells = 22 * DoublePairStore::k_numberOfSeries;
  constexpr static int k_numberOfTitleCells = 4;
  static constexpr int k_titleCellType = 0;
  static constexpr int k_editableCellType = 1;

  class ContentView : public View , public Responder {
  public:
    ContentView(DoublePairStore * store, Responder * parentResponder, TableViewDataSource * dataSource, SelectableTableViewDataSource * selectionDataSource, TextFieldDelegate * textFieldDelegate);
   StoreSelectableTableView * dataView() { return &m_dataView; }
   BufferTextViewWithTextField * formulaInputView() { return &m_formulaInputView; }
   void displayFormulaInput(bool display);
  // Responder
  void didBecomeFirstResponder() override;
  private:
    static constexpr KDCoordinate k_margin = 8;
    static constexpr KDCoordinate k_scrollBarMargin = Metric::CommonRightMargin;
    static constexpr KDCoordinate k_formulaInputHeight = 31;
    int numberOfSubviews() const override { return 1 + m_displayFormulaInputView; }
    View * subviewAtIndex(int index) override;
    void layoutSubviews() override;
    KDRect formulaFrame() const;
    StoreSelectableTableView m_dataView;
    BufferTextViewWithTextField m_formulaInputView;
    bool m_displayFormulaInputView;
  };

  Responder * tabController() const override;
  SelectableTableView * selectableTableView() override;
  View * loadView() override;
  void unloadView(View * view) override;
  bool cellAtLocationIsEditable(int columnIndex, int rowIndex) override;
  bool setDataAtLocation(double floatBody, int columnIndex, int rowIndex) override;
  double dataAtLocation(int columnIndex, int rowIndex) override;
  int numberOfElements() override;
  int maxNumberOfElements() const override;
  virtual HighlightCell * titleCells(int index) = 0;
  char m_draftTextBuffer[TextField::maxBufferSize()];
  int seriesAtColumn(int column) const { return column / DoublePairStore::k_numberOfColumnsPerSeries; }
  bool privateFillColumnWithFormula(Poincare::Expression * formula, Poincare::Expression::isVariableTest isVariable);
  virtual StoreParameterController * storeParameterController() = 0;
  StoreCell * m_editableCells[k_maxNumberOfEditableCells];
  DoublePairStore * m_store;
private:
  bool cellShouldBeTransparent(int i, int j);
  ContentView * contentView() { return static_cast<ContentView *>(view()); }
};

}

#endif
