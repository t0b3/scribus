#include "cmddoc.h"
#include "cmdutil.h"
#include "units.h"
#include "documentinformation.h"

/*
newDocument(size, margins, orientation, firstPageNumber,
            unit, pagesType, firstPageOrder)*/
PyObject *scribus_newdocument(PyObject* /* self */, PyObject* args)
{
	double topMargin, bottomMargin, leftMargin, rightMargin;
	double pageWidth, pageHeight;
	int orientation, firstPageNr, unit, pagesType, facingPages, firstPageOrder;

	PyObject *p, *m;

	if ((!PyArg_ParseTuple(args, "OOiiiii", &p, &m, &orientation,
											&firstPageNr, &unit,
											&pagesType,
											&firstPageOrder)) ||
						(!PyArg_ParseTuple(p, "dd", &pageWidth, &pageHeight)) ||
						(!PyArg_ParseTuple(m, "dddd", &leftMargin, &rightMargin,
												&topMargin, &bottomMargin)))
		return NULL;

	if (pagesType == 0)
	{
		facingPages = 0;
		firstPageOrder = 0;
	}
	else
		facingPages = 1;
	// checking the bounds
	if (pagesType < firstPageOrder)
	{
		PyErr_SetString(ScribusException, QObject::tr("firstPageOrder is bigger than allowed.","python error"));
		return NULL;
	}

	pageWidth = value2pts(pageWidth, unit);
	pageHeight = value2pts(pageHeight, unit);
	if (orientation == 1)
	{
		double x = pageWidth;
		pageWidth = pageHeight;
		pageHeight = x;
	}
	leftMargin = value2pts(leftMargin, unit);
	rightMargin = value2pts(rightMargin, unit);
	topMargin = value2pts(topMargin, unit);
	bottomMargin = value2pts(bottomMargin, unit);

	bool ret = ScMW->doFileNew(pageWidth, pageHeight,
								topMargin, leftMargin, rightMargin, bottomMargin,
								// autoframes. It's disabled in python
								// columnDistance, numberCols, autoframes,
								0, 1, false,
								pagesType, unit, firstPageOrder,
								orientation, firstPageNr, "Custom");
	ScMW->doc->pageSets[pagesType].FirstPage = firstPageOrder;

	return PyInt_FromLong(static_cast<long>(ret));
}

PyObject *scribus_newdoc(PyObject* /* self */, PyObject* args)
{
	qDebug("WARNING: newDoc() procedure is obsolete, it will be removed in a forthcoming release. Use newDocument() instead.");
	double b, h, lr, tpr, btr, rr, ebr;
	int unit, ds, fsl, fNr, ori;
	PyObject *p, *m;
	if ((!PyArg_ParseTuple(args, "OOiiiii", &p, &m, &ori, &fNr, &unit, &ds, &fsl)) ||
	        (!PyArg_ParseTuple(p, "dd", &b, &h)) ||
	        (!PyArg_ParseTuple(m, "dddd", &lr, &rr, &tpr, &btr)))
		return NULL;
	b = value2pts(b, unit);
	h = value2pts(h, unit);
	if (ori == 1)
	{
		ebr = b;
		b = h;
		h = ebr;
	}
	/*! \todo Obsolete! In the case of no facing pages use only firstpageleft
	scripter is not new-page-size ready.
	What is it: don't allow to use wrong FSL constant in the case of
	onesided document. */
	if (ds!=1 && fsl>0)
		fsl = 0;
	// end of hack
	tpr = value2pts(tpr, unit);
	lr = value2pts(lr, unit);
	rr = value2pts(rr, unit);
	btr = value2pts(btr, unit);
	bool ret = ScMW->doFileNew(b, h, tpr, lr, rr, btr, 0, 1, false, ds, unit, fsl, ori, fNr, "Custom");
	//	qApp->processEvents();
	return PyInt_FromLong(static_cast<long>(ret));
}

PyObject *scribus_setmargins(PyObject* /* self */, PyObject* args)
{
	double lr, tpr, btr, rr;
	if (!PyArg_ParseTuple(args, "dddd", &lr, &rr, &tpr, &btr))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	tpr = ValueToPoint(tpr);
	lr = ValueToPoint(lr);
	rr = ValueToPoint(rr);
	btr = ValueToPoint(btr);
	ScMW->doc->resetPage(tpr, lr, rr, btr, ScMW->doc->currentPageLayout);
	ScMW->view->reformPages();
	ScMW->doc->setModified(true);
	ScMW->view->GotoPage(ScMW->doc->currentPageNumber());
	ScMW->view->DrawNew();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *scribus_closedoc(PyObject* /* self */)
{
	if(!checkHaveDocument())
		return NULL;
	ScMW->doc->setModified(false);
	bool ret = ScMW->slotFileClose();
	qApp->processEvents();
	return PyInt_FromLong(static_cast<long>(ret));
}

PyObject *scribus_havedoc(PyObject* /* self */)
{
	return PyInt_FromLong(static_cast<long>(ScMW->HaveDoc));
}

PyObject *scribus_opendoc(PyObject* /* self */, PyObject* args)
{
	char *Name;
	if (!PyArg_ParseTuple(args, "es", "utf-8", &Name))
		return NULL;
	bool ret = ScMW->loadDoc(QString::fromUtf8(Name));
	if (!ret)
	{
		PyErr_SetString(ScribusException, QObject::tr("Failed to open document.","python error"));
		return NULL;
	}
	Py_INCREF(Py_True); // compatibility: return true, not none, on success
	return Py_True;
}

PyObject *scribus_savedoc(PyObject* /* self */)
{
	if(!checkHaveDocument())
		return NULL;
	ScMW->slotFileSave();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *scribus_savedocas(PyObject* /* self */, PyObject* args)
{
	char *Name;
	if (!PyArg_ParseTuple(args, "es", "utf-8", &Name))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	bool ret = ScMW->DoFileSave(QString::fromUtf8(Name));
	if (!ret)
	{
		PyErr_SetString(ScribusException, QObject::tr("Failed to save document.","python error"));
		return NULL;
	}
	Py_INCREF(Py_True); // compatibility: return true, not none, on success
	return Py_True;
}

PyObject *scribus_setinfo(PyObject* /* self */, PyObject* args)
{
	char *Author;
	char *Title;
	char *Desc;
	// z means string, but None becomes a NULL value. QString()
	// will correctly handle NULL.
	if (!PyArg_ParseTuple(args, "zzz", &Author, &Title, &Desc))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	ScMW->doc->documentInfo.setAuthor(QString::fromUtf8(Author));
	ScMW->doc->documentInfo.setTitle(QString::fromUtf8(Title));
	ScMW->doc->documentInfo.setComments(QString::fromUtf8(Desc));
	ScMW->slotDocCh();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *scribus_setunit(PyObject* /* self */, PyObject* args)
{
	int e;
	if (!PyArg_ParseTuple(args, "i", &e))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	if ((e < 0) || (e > 3))
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("Unit out of range. Use one of the scribus.UNIT_* constants.","python error"));
		return NULL;
	}
	ScMW->slotChangeUnit(e);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *scribus_getunit(PyObject* /* self */)
{
	if(!checkHaveDocument())
		return NULL;
	return PyInt_FromLong(static_cast<long>(ScMW->doc->unitIndex()));
}

PyObject *scribus_loadstylesfromfile(PyObject* /* self */, PyObject *args)
{
	char *fileName;
	if (!PyArg_ParseTuple(args, "es", "utf-8", &fileName))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	ScMW->doc->loadStylesFromFile(QString::fromUtf8(fileName));
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *scribus_setdoctype(PyObject* /* self */, PyObject* args)
{
	int fp, fsl;
	if (!PyArg_ParseTuple(args, "ii", &fp, &fsl))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	if (ScMW->doc->currentPageLayout = fp)
		ScMW->doc->pageSets[ScMW->doc->currentPageLayout].FirstPage = fsl;
	ScMW->view->reformPages();
	ScMW->view->GotoPage(ScMW->doc->currentPageNumber()); // is this needed?
	ScMW->view->DrawNew();   // is this needed?
	//CB TODO ScMW->pagePalette->RebuildPage(); // is this needed?
	ScMW->slotDocCh();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *scribus_closemasterpage(PyObject* /* self */)
{
	if(!checkHaveDocument())
		return NULL;
	ScMW->view->hideMasterPage();
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *scribus_masterpagenames(PyObject* /* self */)
{
	if(!checkHaveDocument())
		return NULL;
	PyObject* names = PyList_New(ScMW->doc->MasterPages.count());
	QMap<QString,int>::const_iterator it(ScMW->doc->MasterNames.constBegin());
	QMap<QString,int>::const_iterator itEnd(ScMW->doc->MasterNames.constEnd());
	int n = 0;
	for ( ; it != itEnd; ++it )
	{
		PyList_SET_ITEM(names, n++, PyString_FromString(it.key().utf8().data()) );
	}
	return names;
}

PyObject *scribus_editmasterpage(PyObject* /* self */, PyObject* args)
{
	char* name = 0;
	if (!PyArg_ParseTuple(args, "es", const_cast<char*>("utf-8"), &name))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	const QString masterPageName(name);
	const QMap<QString,int>& masterNames(ScMW->doc->MasterNames);
	const QMap<QString,int>::const_iterator it(masterNames.find(masterPageName));
	if ( it == masterNames.constEnd() )
	{
		PyErr_SetString(PyExc_ValueError, "Master page not found");
		return NULL;
	}
	ScMW->view->showMasterPage(*it);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* scribus_createmasterpage(PyObject* /* self */, PyObject* args)
{
	char* name = 0;
	if (!PyArg_ParseTuple(args, "es", const_cast<char*>("utf-8"), &name))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	const QString masterPageName(name);
	if (ScMW->doc->MasterNames.contains(masterPageName))
	{
		PyErr_SetString(PyExc_ValueError, "Master page already exists");
		return NULL;
	}
	ScMW->doc->addMasterPage(ScMW->doc->MasterPages.count(), masterPageName);
	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* scribus_deletemasterpage(PyObject* /* self */, PyObject* args)
{
	char* name = 0;
	if (!PyArg_ParseTuple(args, "es", const_cast<char*>("utf-8"), &name))
		return NULL;
	if(!checkHaveDocument())
		return NULL;
	const QString masterPageName(name);
	if (!ScMW->doc->MasterNames.contains(masterPageName))
	{
		PyErr_SetString(PyExc_ValueError, "Master page does not exist");
		return NULL;
	}
	if (masterPageName == "Normal")
	{
		PyErr_SetString(PyExc_ValueError, "Can not delete the Normal master page");
		return NULL;
	}
	bool oldMode = ScMW->doc->masterPageMode();
	ScMW->doc->setMasterPageMode(true);
	ScMW->DeletePage2(ScMW->doc->MasterNames[masterPageName]);
	ScMW->doc->setMasterPageMode(oldMode);
	Py_INCREF(Py_None);
	return Py_None;
}
