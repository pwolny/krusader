#ifndef __krinterview__
#define __krinterview__

#include "krview.h"

#include <QTreeView>
#include <QVector>

class KrVfsModel;
class KrInterViewItem;
class QMouseEvent;
class QKeyEvent;
class KrMouseHandler;

class KrInterView : public QTreeView, public KrView {
	friend class KrInterViewItem;
	Q_OBJECT

public:
	KrInterView( QWidget *parent, bool &left, KConfig *cfg = krConfig );
	virtual ~KrInterView();

	virtual void addItems(vfs* v, bool addUpDir = true);
	virtual KrViewItem* findItemByName(const QString &name);
	virtual QString getCurrentItem() const;
	virtual KrViewItem* getCurrentKrViewItem();
	virtual KrViewItem* getFirst();
	virtual KrViewItem* getKrViewItemAt(const QPoint &vp);
	virtual KrViewItem* getLast();
	virtual KrViewItem* getNext(KrViewItem *current);
	virtual KrViewItem* getPrev(KrViewItem *current);
	virtual void makeItemVisible(const KrViewItem *item);
	virtual KrViewItem* preAddItem(vfile *vf);
	virtual bool preDelItem(KrViewItem *item);
	virtual void redraw();
	virtual void refreshColors();
	virtual void restoreSettings();
	virtual void saveSettings();
	virtual void setCurrentItem(const QString& name);
	virtual void setCurrentKrViewItem(KrViewItem *current);
	virtual void sort();
	virtual void clear();
	virtual void updateView();
	virtual void updateItem(KrViewItem* item);
	virtual QModelIndex getCurrentIndex() { return currentIndex(); }
	virtual bool isSelected( const QModelIndex &ndx ) { return selectionModel()->isSelected( ndx ); }
	virtual void selectRegion( KrViewItem *, KrViewItem *, bool );
	KrInterViewItem * getKrInterViewItem( const QModelIndex & );
	
	static KrView* create( QWidget *parent, bool &left, KConfig *cfg ) { return new KrInterView( parent, left, cfg ); }
	
	virtual void prepareForActive();
	virtual void prepareForPassive();
	virtual bool ensureVisibilityAfterSelect() { return false; }
	virtual int  itemsPerPage();
	
protected slots:
	void slotMakeCurrentVisible();
	virtual void renameCurrentItem();
	
protected:
	virtual void setup();
	virtual void initOperator();
	
	virtual void keyPressEvent( QKeyEvent *e );
	virtual void mousePressEvent ( QMouseEvent * );
	virtual void mouseReleaseEvent ( QMouseEvent * );
	virtual void mouseDoubleClickEvent ( QMouseEvent *ev );
	virtual void mouseMoveEvent ( QMouseEvent * );
	virtual void wheelEvent ( QWheelEvent * );
	virtual bool event( QEvent * e );
	
private:
	KrVfsModel *_model;
	KrMouseHandler *_mouseHandler;
	QHash<vfile *,KrInterViewItem*> _itemHash;
};
#endif // __krinterview__
