/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2009 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "taxBreakdown.h"

#include <QSqlError>
#include <QVariant>

#include <metasql.h>

#include "taxCache.h"
#include "taxDetail.h"

taxBreakdown::taxBreakdown(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  _currency->setLabel(_currencyLit);
  _taxcurrency->setLabel(_taxcurrencyLit);

  connect(_adjTaxLit,    SIGNAL(leftClickedURL(QString)),this, SLOT(sAdjTaxDetail()));
  connect(_freightTaxLit,SIGNAL(leftClickedURL(QString)),this, SLOT(sFreightTaxDetail()));
  connect(_lineTaxLit,   SIGNAL(leftClickedURL(QString)),this, SLOT(sLineTaxDetail()));
  connect(_save,         SIGNAL(clicked()),              this, SLOT(sSave()));
  connect(_totalTaxLit,  SIGNAL(leftClickedURL(QString)),this, SLOT(sTotalTaxDetail()));

  _orderid	= -1;
  _ordertype	= "";
  _taxCache.clear();
}

taxBreakdown::~taxBreakdown()
{
  // no need to delete child widgets, Qt does it all for us
}

void taxBreakdown::languageChange()
{
  retranslateUi(this);
}

SetResponse taxBreakdown::set(const ParameterList& pParams)
{
  QVariant param;
  bool	   valid;

  param = pParams.value("order_id", &valid);
  if (valid)
    _orderid = param.toInt();

  param = pParams.value("order_type", &valid);
  if (valid)
    _ordertype = param.toString();

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if(param.toString() == "view")
    {
      _mode = cView;
      _save->hide();
    }
    else
      _mode = cEdit;
  }

  ParameterList params;
  if (_ordertype == "I")
  {
    _currencyLit->setText(tr("Invoice Currency:"));
    _header->setText(tr("Tax Breakdown for Invoice:"));
    _totalLit->setText(tr("Invoice Total:"));

    params.append("invchead_id", _orderid);
  }
  else if (_ordertype == "S")
  {
    _currencyLit->setText(tr("Sales Order Currency:"));
    _header->setText(tr("Tax Breakdown for Sales Order:"));
    _totalLit->setText(tr("Sales Order Total:"));

    _adjTaxLit->setVisible(false);
    _adjTax->setVisible(false);

    params.append("cohead_id", _orderid);
  }
  else if (_ordertype == "Q")
  {
    _currencyLit->setText(tr("Quote Currency:"));
    _header->setText(tr("Tax Breakdown for Quote:"));
    _totalLit->setText(tr("Quote Total:"));

    _adjTaxLit->setVisible(false);
    _adjTax->setVisible(false);

    params.append("quhead_id", _orderid);
  }
  else if (_ordertype == "RA")
  {
    _currencyLit->setText(tr("Return Authorization Currency:"));
    _header->setText(tr("Tax Breakdown for Return:"));
    _totalLit->setText(tr("Return Total:"));

    _adjTaxLit->setVisible(false);
    _adjTax->setVisible(false);

    params.append("rahead_id", _orderid);
  }
  else if (_ordertype == "B")
  {
    _currencyLit->setText(tr("Billing Currency:"));
    _header->setText(tr("Tax Breakdown for Billing Order:"));
    _totalLit->setText(tr("Billing Total:"));

    params.append("cobmisc_id", _orderid);
  }
  else if (_ordertype == "CM")
  {
    _currencyLit->setText(tr("Credit Memo Currency:"));
    _header->setText(tr("Tax Breakdown for Credit Memo:"));
    _totalLit->setText(tr("Credit Memo Total:"));

    params.append("cmhead_id", _orderid);
  }
  else if (_ordertype == "TO")
  {
    _currencyLit->setText(tr("Transfer Order Currency:"));
    _header->setText(tr("Tax Breakdown for Transfer Order:"));
    _totalLit->setText(tr("Transfer Order Total:"));

    params.append("tohead_id", _orderid);
  }

  QString sql =
	"<? if exists(\"invchead_id\") ?>"
	/*"  SELECT invchead_invcnumber AS number,"
	"         invchead_taxzone_id AS taxzone_id,"
	"         invchead_curr_id AS curr_id,"
	"         invchead_tax_curr_id AS tax_curr_id,"
	"         invchead_invcdate AS date,"
	"         SUM(ROUND(((invcitem_billed * invcitem_qty_invuomratio) * (invcitem_price) / "
	"                  (CASE WHEN(item_id IS NULL) THEN 1 "
	"			ELSE invcitem_price_invuomratio END)), 2)) AS line,"
	"         SUM(invcitemtax.taxhist_tax) AS total_tax,"
	"         invchead_freight AS freight,"
	"         invchead_freighttaxtype_id AS freightTax,"
	"         SUM(a.taxhist_tax) AS freighttaxamt,"
	"         invchead_adjtaxtype_id AS adjTax,"
	"         SUM(b.taxhist_tax) AS adjtaxamt "
	"  FROM invcitem LEFT OUTER JOIN"
	"       item ON (invcitem_item_id=item_id) LEFT OUTER JOIN invcitemtax ON (taxhist_parent_id = invcitem_id)"
	"       LEFT OUTER JOIN invchead ON (invcitem_invchead_id=invchead_id) LEFT OUTER JOIN invcheadtax a ON (a.taxhist_parent_id = invchead_id AND a.taxhist_taxtype_id = getfreighttaxtypeid()) "
	"       LEFT OUTER JOIN invcheadtax b ON (b.taxhist_parent_id = invchead_id AND b.taxhist_taxtype_id = getadjustmenttaxtypeid()) "  
	"  WHERE (invcitem_invchead_id=<? value(\"invchead_id\") ?>)"
	"  GROUP BY number, taxzone_id, curr_id, tax_curr_id,"
	"         date, freight, freightTax, adjTax;"*/

	"SELECT head.*, linetax.*, freighttax.*, adjtax.* "
    "FROM (SELECT invchead_invcnumber AS number, invchead_taxzone_id AS taxzone_id, "
	"         invchead_curr_id AS curr_id, invchead_tax_curr_id AS tax_curr_id, invchead_invcdate AS date, "
	"      SUM(ROUND(((invcitem_billed * invcitem_qty_invuomratio) * (invcitem_price) / "
    "     (CASE WHEN(item_id IS NULL) THEN 1 ELSE invcitem_price_invuomratio END)), 2)) AS line, "
    "     invchead_freight AS freight, "
    "      invchead_freighttaxtype_id AS freightTax, invchead_adjtaxtype_id AS adjTax "
    "      FROM invcitem LEFT OUTER JOIN item ON (invcitem_item_id=item_id) "
	"      LEFT OUTER JOIN invchead ON (invcitem_invchead_id=invchead_id) "
    "	  WHERE (invcitem_invchead_id=<? value(\"invchead_id\") ?>) "
    "	  GROUP BY number, taxzone_id, curr_id, tax_curr_id, date, freight, freightTax, adjTax) head, "
	"     (SELECT SUM(invcitemtax.taxhist_tax) AS total_tax FROM invcitemtax LEFT OUTER JOIN invcitem ON "
	"      (taxhist_parent_id = invcitem_id) WHERE (invcitem_invchead_id=<? value(\"invchead_id\") ?>)) linetax,"
    "    (SELECT  SUM(taxhist_tax) AS freighttaxamt FROM invchead LEFT OUTER JOIN invcheadtax  "
	"    ON (taxhist_parent_id = invchead_id AND taxhist_taxtype_id = getfreighttaxtypeid()) "
    "    WHERE invchead_id=<? value(\"invchead_id\") ?>) freighttax, "
    "   (SELECT  SUM(taxhist_tax) AS adjtaxamt FROM invchead LEFT OUTER JOIN invcheadtax ON "
	"   (taxhist_parent_id = invchead_id AND taxhist_taxtype_id = getadjustmenttaxtypeid()) "
    "    WHERE invchead_id=<? value(\"invchead_id\") ?>) adjtax;"
	"<? elseif exists(\"cmhead_id\") ?>"
	"  SELECT cmhead_number AS number,"
	"         cmhead_taxauth_id AS taxauth_id,"
	"         cmhead_curr_id AS curr_id,"
	"         cmhead_tax_curr_id AS tax_curr_id,"
	"         cmhead_docdate AS date,"
	"         SUM(ROUND((cmitem_qtycredit * cmitem_qty_invuomratio) * (cmitem_unitprice / "
	"                  (CASE WHEN(item_id IS NULL) THEN 1 "
	"			ELSE cmitem_price_invuomratio END)), 2)) AS line,"
	"         SUM(cmitem_tax_ratea) AS linea,"
	"         SUM(cmitem_tax_rateb) AS lineb,"
	"         SUM(cmitem_tax_ratec) AS linec,"
	"         cmhead_freight AS freight,"
	"         cmhead_freighttax_id AS freightTax,"
	"         cmhead_freighttax_pcta AS freightPcta,"
	"         cmhead_freighttax_pctb AS freightPctb,"
	"         cmhead_freighttax_pctc AS freightPctc,"
	"         cmhead_freighttax_ratea AS freighta,"
	"         cmhead_freighttax_rateb AS freightb,"
	"         cmhead_freighttax_ratec AS freightc,"
	"         cmhead_adjtax_id AS adjTax,"
	"         cmhead_adjtax_pcta AS adjPcta,"
	"         cmhead_adjtax_pctb AS adjPctb,"
	"         cmhead_adjtax_pctc AS adjPctc,"
	"         cmhead_adjtax_ratea AS adja,"
	"         cmhead_adjtax_rateb AS adjb,"
	"         cmhead_adjtax_ratec AS adjc"
	"  FROM cmitem JOIN"
	"       cmhead ON (cmitem_cmhead_id=cmhead_id) LEFT OUTER JOIN"
	"       itemsite ON (cmitem_itemsite_id=itemsite_id) LEFT OUTER JOIN"
	"       item ON (itemsite_item_id=item_id)"
	"  WHERE (cmitem_cmhead_id=<? value(\"cmhead_id\") ?>)"
	"  GROUP BY number, taxauth_id, curr_id, tax_curr_id,"
	"         date, freight, freightTax,"
	"         freightPcta, freightPctb, freightPctc,"
	"         freighta, freightb, freightc,"
	"         adjTax, adjPcta, adjPctb, adjPctc, adja, adjb, adjc;"
	"<? elseif exists(\"cobmisc_id\") ?>"
	" SELECT head.*, linetax.*, frighttax.*, adjtax.*  " 
    " FROM (SELECT cohead_number AS number, cobmisc_taxzone_id AS taxzone_id, "
	"       cobmisc_curr_id AS curr_id, cobmisc_tax_curr_id AS tax_curr_id, "
    "       cobmisc_invcdate AS date, SUM(ROUND((cobill_qty * coitem_price) / "
    "       (CASE WHEN(item_id IS NULL) THEN 1 ELSE coitem_price_invuomratio END), 2)) AS line, "
    "       cobmisc_freight AS freight, cobmisc_freighttaxtype_id AS freightTax, "
    "       cobmisc_adjtaxtype_id AS adjTax	"         
    "      FROM coitem JOIN cobill ON (cobill_coitem_id=coitem_id) "
    "	        JOIN cobmisc ON (cobill_cobmisc_id=cobmisc_id) "
    "	        JOIN cohead  ON (coitem_cohead_id=cohead_id) LEFT OUTER JOIN "
    "	       itemsite ON (coitem_itemsite_id=itemsite_id) LEFT OUTER JOIN "
    "	       item ON (itemsite_item_id=item_id) "
    "	   WHERE (cobill_cobmisc_id=<? value(\"cobmisc_id\") ?>) "
    "	  GROUP BY number, cobmisc_taxzone_id, curr_id, tax_curr_id, "
    "	         date, freight, freightTax,adjTax) head, "
    "     (SELECT SUM(cobilltax.taxhist_tax) AS total_tax FROM cobilltax LEFT OUTER JOIN "
	"       cobill ON (cobill_id=taxhist_parent_id) "
    "      WHERE (cobill_cobmisc_id=<? value(\"cobmisc_id\") ?>)) linetax, "
    "     (SELECT SUM(taxhist_tax) AS freighttaxamt FROM cobmisctax LEFT OUTER JOIN "
	"       cobmisc ON (taxhist_parent_id = cobmisc_id AND taxhist_taxtype_id = getfreighttaxtypeid()) "
    "      WHERE (cobmisc_id=<? value(\"cobmisc_id\") ?>)) frighttax, "
    "     ( SELECT  SUM(taxhist_tax) AS adjtaxamt FROM cobmisctax LEFT OUTER JOIN "
	"        cobmisc ON (taxhist_parent_id = cobmisc_id AND taxhist_taxtype_id = getadjustmenttaxtypeid()) "
    "        WHERE (cobmisc_id=<? value(\"cobmisc_id\") ?>)) adjtax;"

	/*"  SELECT cohead_number AS number,"
	"         cobmisc_taxzone_id AS taxzone_id,"
	"         cobmisc_curr_id AS curr_id,"
	"         cobmisc_tax_curr_id AS tax_curr_id,"
	"         cobmisc_invcdate AS date,"
	"         SUM(ROUND((cobill_qty * coitem_price) / "
	"                  (CASE WHEN(item_id IS NULL) THEN 1 "
	"			ELSE coitem_price_invuomratio END), 2)) AS line,"
	"         SUM(cobilltax.taxhist_tax) AS total_tax,"
	"         cobmisc_freight AS freight,"
	"         cobmisc_freighttaxtype_id AS freightTax,"
	"         SUM(a.taxhist_tax) AS freighttaxamt,"
	"         cobmisc_adjtaxtype_id AS adjTax,"
	"         SUM(b.taxhist_tax) AS adjtaxamt "
	"  FROM coitem JOIN cobill ON (cobill_coitem_id=coitem_id) "
	"        LEFT OUTER JOIN cobilltax ON (taxhist_parent_id = cobill_id) "
	"       JOIN cobmisc ON (cobill_cobmisc_id=cobmisc_id) "
	"      LEFT OUTER JOIN cobmisctax a ON (a.taxhist_parent_id = cobmisc_id AND a.taxhist_taxtype_id = getfreighttaxtypeid()) "
	"       LEFT OUTER JOIN cobmisctax b ON (b.taxhist_parent_id = cobmisc_id AND b.taxhist_taxtype_id = getadjustmenttaxtypeid()) "
	"       JOIN cohead  ON (coitem_cohead_id=cohead_id) LEFT OUTER JOIN"
	"       itemsite ON (coitem_itemsite_id=itemsite_id) LEFT OUTER JOIN"
	"       item ON (itemsite_item_id=item_id) "
	"  WHERE (cobill_cobmisc_id=<? value(\"cobmisc_id\") ?>)"
	"  GROUP BY number, cobmisc_taxzone_id, curr_id, tax_curr_id,"
	"         date, freight, freightTax,adjTax;"*/
	"<? elseif exists(\"cohead_id\") ?>"
	"  SELECT cohead_number AS number,"
	"         cohead_taxzone_id AS taxzone_id,"
	"         cohead_curr_id AS curr_id,"
	"         cohead_curr_id AS tax_curr_id,"
	"         cohead_orderdate AS date,"
	"         SUM(ROUND((coitem_qtyord * coitem_qty_invuomratio) * (coitem_price / coitem_price_invuomratio),"
	"                    2)) AS line,"
	"         SUM(ROUND(calculateTax(cohead_taxzone_id, coitem_taxtype_id, cohead_orderdate, cohead_curr_id, ROUND((coitem_qtyord * coitem_qty_invuomratio) * (coitem_price /"
	"                        coitem_price_invuomratio), 2)),2)) as total_tax, "  
	"         cohead_freight AS freight,"
	"         taxtype_id AS freightTax,"
	"         ROUND(calculateTax(cohead_taxzone_id, getfreighttaxtypeid(), cohead_orderdate, cohead_curr_id, cohead_freight), 2) AS freighttaxamt, "
	"         NULL AS adjTax,"
	"         NULL AS adjPcta,"
	"         NULL AS adjPctb,"
	"         NULL AS adjPctc,"
	"         NULL AS adja,"
	"         NULL AS adjb,"
	"         NULL AS adjc"
	"  FROM coitem, item, itemsite, cohead LEFT OUTER JOIN"
	"       taxzone ON (cohead_taxzone_id=taxzone_id) LEFT OUTER JOIN"
	"       taxtype ON (taxtype_id=getFreightTaxTypeId())"
	"  WHERE ((coitem_cohead_id=<? value(\"cohead_id\") ?>)"
	"    AND  (coitem_cohead_id=cohead_id)"
	"    AND  (coitem_itemsite_id=itemsite_id)"
	"    AND  (itemsite_item_id=item_id))"
	"  GROUP BY number, cohead_taxzone_id, curr_id, tax_curr_id,"
	"         date, freight, freightTax,freighttaxamt;"
	"<? elseif exists(\"quhead_id\") ?>"
	"  SELECT quhead_number AS number,"
	"         quhead_taxzone_id AS taxzone_id,"
	"         quhead_curr_id AS curr_id,"
	"         quhead_curr_id AS tax_curr_id,"
	"         quhead_quotedate AS date,"
	"         SUM(ROUND((quitem_qtyord * quitem_qty_invuomratio) * (quitem_price / quitem_price_invuomratio),"
	"                    2)) AS line,"
    "         SUM(ROUND(calculateTax(quhead_taxzone_id, quitem_taxtype_id, quhead_quotedate, quhead_curr_id, ROUND((quitem_qtyord * quitem_qty_invuomratio) * (quitem_price /"
	"                        quitem_price_invuomratio), 2)),2)) as total_tax, " 
	"         quhead_freight AS freight,"
	"         taxtype_id AS freightTax,"
	"         ROUND(calculateTax(quhead_taxzone_id, getfreighttaxtypeid(), quhead_quotedate, quhead_curr_id, quhead_freight), 2) AS freighttaxamt, "
	"         NULL AS adjTax,"
	"         NULL AS adjPcta,"
	"         NULL AS adjPctb,"
	"         NULL AS adjPctc,"
	"         NULL AS adja,"
	"         NULL AS adjb,"
	"         NULL AS adjc"
	"  FROM quitem, item, quhead LEFT OUTER JOIN"
	"       taxzone ON (quhead_taxzone_id=taxzone_id) LEFT OUTER JOIN"
	"       taxtype ON (taxtype_id=getFreightTaxTypeId())"
	"  WHERE ((quitem_quhead_id=<? value(\"quhead_id\") ?>)"
	"    AND  (quitem_quhead_id=quhead_id)"
	"    AND  (quitem_item_id=item_id))"
	"  GROUP BY number, quhead_taxzone_id, curr_id, tax_curr_id,"
	"         date, freight, freightTax,freighttaxamt;"
	"<? elseif exists(\"rahead_id\") ?>"
	"  SELECT rahead_number AS number,"
	"         rahead_taxauth_id AS taxauth_id,"
	"         rahead_curr_id AS curr_id,"
	"         rahead_curr_id AS tax_curr_id,"
	"         rahead_authdate AS date,"
	"         SUM(ROUND((raitem_qtyauthorized * raitem_qty_invuomratio) * (raitem_unitprice / raitem_price_invuomratio),"
	"                    2)) AS line,"
	"         SUM(ROUND((calculateTax(raitem_tax_id,"
	"                        (ROUND((raitem_qtyauthorized * raitem_qty_invuomratio) * (raitem_unitprice /"
	"                        raitem_price_invuomratio), 2)), 0, 'A')), 2)) AS linea,"
	"         SUM(ROUND((calculateTax(raitem_tax_id,"
	"                        (ROUND((raitem_qtyauthorized * raitem_qty_invuomratio) * (raitem_unitprice /"
	"                        raitem_price_invuomratio), 2)), 0, 'B')), 2)) AS lineb,"
	"         SUM(ROUND((calculateTax(raitem_tax_id,"
	"                        (ROUND((raitem_qtyauthorized * raitem_qty_invuomratio) * (raitem_unitprice /"
	"                        raitem_price_invuomratio), 2)), 0, 'C')), 2)) AS linec,"
	"         rahead_freight AS freight,"
	"         tax_id AS freightTax,"
	"         tax_ratea AS freightPcta,"
	"         tax_rateb AS freightPctb,"
	"         tax_ratec AS freightPctc,"
	"         ROUND(calculateTax(tax_id, ROUND(rahead_freight, 2),"
	"                         0, 'A'), 2) AS freighta,"
	"         ROUND(calculateTax(tax_id, ROUND(rahead_freight, 2),"
	"                         0, 'B'), 2) AS freightb,"
	"         ROUND(calculateTax(tax_id, ROUND(rahead_freight, 2),"
	"                         0, 'C'), 2) AS freightc,"
	"         NULL AS adjTax,"
	"         NULL AS adjPcta,"
	"         NULL AS adjPctb,"
	"         NULL AS adjPctc,"
	"         NULL AS adja,"
	"         NULL AS adjb,"
	"         NULL AS adjc"
	"  FROM raitem, rahead LEFT OUTER JOIN"
	"       taxauth ON (rahead_taxauth_id=taxauth_id) LEFT OUTER JOIN"
	"       tax ON (tax_id=getFreightTaxSelection(taxauth_id))"
	"  WHERE ((raitem_rahead_id=<? value(\"rahead_id\") ?>)"
	"    AND  (raitem_rahead_id=rahead_id))"
	"  GROUP BY number, rahead_taxauth_id, curr_id, tax_curr_id,"
	"         date, freight, freightTax,"
	"         freightPcta, freightPctb, freightPctc;"
	"<? elseif exists(\"tohead_id\") ?>"
	"  SELECT tohead_number AS number,"
	"         tohead_taxauth_id AS taxauth_id,"
	"         tohead_freight_curr_id AS curr_id,"
	"         tohead_freight_curr_id AS tax_curr_id,"
	"         tohead_orderdate AS date,"
	"         SUM(ROUND(toitem_freight, 2)) AS line,"
	"         SUM(ROUND((toitem_freighttax_ratea), 2)) AS linea,"
	"         SUM(ROUND((toitem_freighttax_rateb), 2)) AS lineb,"
	"         SUM(ROUND((toitem_freighttax_ratec), 2)) AS linec,"
	"         tohead_freight AS freight,"
	"         tohead_freighttax_id AS freightTax,"
	"         tohead_freighttax_pcta AS freightPcta,"
	"         tohead_freighttax_pctb AS freightPctb,"
	"         tohead_freighttax_pctc AS freightPctc,"
	"         ROUND(tohead_freighttax_ratea, 2) AS freighta,"
	"         ROUND(tohead_freighttax_rateb, 2) AS freightb,"
	"         ROUND(tohead_freighttax_ratec, 2) AS freightc,"
	"         NULL AS adjTax,"
	"         NULL AS adjPcta,"
	"         NULL AS adjPctb,"
	"         NULL AS adjPctc,"
	"         NULL AS adja,"
	"         NULL AS adjb,"
	"         NULL AS adjc"
	"  FROM toitem, tohead "
	"  WHERE ((toitem_tohead_id=<? value(\"tohead_id\") ?>)"
	"    AND  (toitem_tohead_id=tohead_id)"
	"    AND  (toitem_status!='X')) "
	"  GROUP BY number, taxauth_id, curr_id, tax_curr_id,"
	"         date,		freight,	freightTax,"
	"         freighta,	freightb,	freightc,"
	"         freightPcta,	freightPctb,	freightPctc;"
	"<? endif ?>";

  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  if (q.first())
  {
    // do dates and currencies first because of signal/slot cascades
    _total->setEffective(q.value("date").toDate());
    _currency->setId(q.value("curr_id").toInt());
    _taxcurrency->setId(q.value("tax_curr_id").toInt());

    // now the rest
    _document->setText(q.value("number").toString());
    _taxzone->setId(q.value("taxzone_id").toInt());
    _line->setLocalValue(q.value("line").toDouble());
    _freight->setLocalValue(q.value("freight").toDouble());
    _pretax->setLocalValue(_line->localValue() + _freight->localValue());

    
	if(_ordertype == "S"  || _ordertype == "Q" || _ordertype == "I" || _ordertype == "B")
      _taxCache.setLine(q.value("total_tax").toDouble(), 0.0, 0.0);
    else
	  _taxCache.setLine(q.value("linea").toDouble(),
		      q.value("lineb").toDouble(),
		      q.value("linec").toDouble());

    if(_ordertype == "S"  || _ordertype == "Q" || _ordertype == "I" || _ordertype == "B")
       _taxCache.setFreightType(q.value("freightTax").toInt());
	else
       _taxCache.setFreightId(q.value("freightTax").toInt()); 
   
   if(_ordertype != "S"  && _ordertype != "Q" &&  _ordertype != "I" && _ordertype != "B") 
   {
       _taxCache.setFreightPct(q.value("freightPcta").toDouble(),
			    q.value("freightPctb").toDouble(),
			    q.value("freightPctc").toDouble());
       _taxCache.setFreight(q.value("freighta").toDouble(),
			 q.value("freightb").toDouble(),
			 q.value("freightc").toDouble());
   }
    
	else
	 _taxCache.setFreight(q.value("freighttaxamt").toDouble(),
			 0,
			 0);

     if(_ordertype == "S"  || _ordertype == "Q" || _ordertype == "I" || _ordertype == "B")
         _taxCache.setAdjType(q.value("adjTax").toInt());
	 else
		 _taxCache.setAdjId(q.value("adjTax").toInt());

	if(_ordertype != "I" && _ordertype != "B") 
	 {
       _taxCache.setAdjPct(q.value("adjPcta").toDouble(),
			q.value("adjPctb").toDouble(),
			q.value("adjPctc").toDouble());
       _taxCache.setAdj(q.value("adja").toDouble(),
		     q.value("adjb").toDouble(),
		     q.value("adjc").toDouble());
	 }
	 else
         _taxCache.setAdj(q.value("adjtaxamt").toDouble(),
		     0,
		     0);

    SetResponse calc = sRecalc();
    if (calc != NoError)
      return calc;
  }
  else if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return UndefinedError;
  }

  return NoError;
}

void taxBreakdown::sSave()
{
  ParameterList params;
  if (_ordertype == "I")
    params.append("invchead_id", _orderid);
  else if (_ordertype == "B")
    params.append("cobmisc_id", _orderid);
  else if (_ordertype == "CM")
    params.append("cmhead_id", _orderid);
  else if (_ordertype == "TO")
    params.append("tohead_id", _orderid);
  else
    accept();

  params.append("adja",	 	_taxCache.adj(0));
  params.append("adjb",	 	_taxCache.adj(1));
  params.append("adjc",	 	_taxCache.adj(2));
  params.append("adjPctA",	_taxCache.adjPct(0));
  params.append("adjPctB",	_taxCache.adjPct(1));
  params.append("adjPctC",	_taxCache.adjPct(2));
  params.append("freighta",	_taxCache.freight(0));
  params.append("freightb",	_taxCache.freight(1));
  params.append("freightc",	_taxCache.freight(2));
  params.append("freightPctA",	_taxCache.freightPct(0));
  params.append("freightPctB",	_taxCache.freightPct(1));
  params.append("freightPctC",	_taxCache.freightPct(2));

  if (_ordertype == "I" || _ordertype == "B")
  {
    if (_taxCache.adjType() > 0)
      params.append("adjTax",	_taxCache.adjType());
  }
  else
  {
    if (_taxCache.adjId() > 0)
     params.append("adjTax",	_taxCache.adjId());
  }


  if (_ordertype == "I" || _ordertype == "B")
  {
    if (_taxCache.freightType() > 0)
      params.append("freightTax", _taxCache.freightType());
  }
  else
  {
    if (_taxCache.freightId() > 0)
      params.append("freightTax", _taxCache.freightId());
  }

  QString sql =
	"<? if exists(\"invchead_id\") ?>"
	"  UPDATE invchead SET "
	"         invchead_adjtaxtype_id=<? value(\"adjTax\") ?> "
	"  WHERE (invchead_id=<? value(\"invchead_id\") ?>) AND "
	"        ((SELECT COUNT(*) FROM invcheadtax WHERE taxhist_parent_id=<? value(\"invchead_id\") ?> "
	"                AND taxhist_taxtype_id = getadjustmenttaxtypeid()) > 0);"
	"<? elseif exists(\"cmhead_id\") ?>"
	"  UPDATE cmhead SET "
	"         cmhead_adjtax_id=<? value(\"adjTax\") ?>,"
	"         cmhead_adjtax_pcta=<? value(\"adjPctA\") ?>,"
	"         cmhead_adjtax_pctb=<? value(\"adjPctB\") ?>,"
	"         cmhead_adjtax_pctc=<? value(\"adjPctC\") ?>,"
	"         cmhead_adjtax_ratea=<? value(\"adja\") ?>,"
	"         cmhead_adjtax_rateb=<? value(\"adjb\") ?>,"
	"         cmhead_adjtax_ratec=<? value(\"adjc\") ?>,"
	"         cmhead_freighttax_id=<? value(\"freightTax\") ?>,"
	"         cmhead_freighttax_pcta=<? value(\"freightPctA\") ?>,"
	"         cmhead_freighttax_pctb=<? value(\"freightPctB\") ?>,"
	"         cmhead_freighttax_pctc=<? value(\"freightPctC\") ?>,"
	"         cmhead_freighttax_ratea=<? value(\"freighta\") ?>,"
	"         cmhead_freighttax_rateb=<? value(\"freightb\") ?>,"
	"         cmhead_freighttax_ratec=<? value(\"freightc\") ?>"
	"  WHERE (cmhead_id=<? value(\"cmhead_id\") ?>)"
	"<? elseif exists(\"cobmisc_id\") ?>"
	"  UPDATE cobmisc SET "
	"         cobmisc_adjtaxtype_id=<? value(\"adjTax\") ?> "
	"  WHERE (cobmisc_id=<? value(\"cobmisc_id\") ?>) AND "
	"        ((SELECT COUNT(*) FROM cobmisctax WHERE taxhist_parent_id=<? value(\"cobmisc_id\") ?> "
	"                AND taxhist_taxtype_id = getadjustmenttaxtypeid()) > 0);"
	"<? elseif exists(\"tohead_id\") ?>"
	"  UPDATE tohead SET "
	"         tohead_freighttax_id=<? value(\"freightTax\") ?>,"
	"         tohead_freighttax_pcta=<? value(\"freightPctA\") ?>,"
	"         tohead_freighttax_pctb=<? value(\"freightPctB\") ?>,"
	"         tohead_freighttax_pctc=<? value(\"freightPctC\") ?>,"
	"         tohead_freighttax_ratea=<? value(\"freighta\") ?>,"
	"         tohead_freighttax_rateb=<? value(\"freightb\") ?>,"
	"         tohead_freighttax_ratec=<? value(\"freightc\") ?>"
	"  WHERE (tohead_id=<? value(\"tohead_id\") ?>)"
	"<? endif ?>";

  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  accept();
}

void taxBreakdown::sAdjTaxDetail()
{
  taxDetail newdlg(this, "", true);
  ParameterList params;

   params.append("curr_id", _taxcurrency->id());
   params.append("date",    _adjTax->effective());
// s/o and quote don't have a place to store changes
  if (cView == _mode)
    params.append("readOnly");

 if (_ordertype == "I" || _ordertype == "B")
 {
   if (_taxCache.adjType() > 0)
      params.append("taxtype_id",  _taxCache.adjType());
   else
   {
     q.exec("SELECT getadjustmenttaxtypeid() as taxtype;");
	 if(q.first())
		 params.append("taxtype_id", q.value("taxtype").toInt());  
   }
   
   params.append("order_type", _ordertype);
   params.append("order_id", _orderid);
   params.append("display_type", "A");
   params.append("adjustment");
   //params.append("readOnly");
   if (newdlg.set(params) == NoError)  
   {
	   newdlg.exec();
	   _taxCache.setAdjType(newdlg.taxtype());
	   sPopulateAdj();
   }
 }
 else
 {
  params.append("tax_id",  _taxCache.adjId());
  params.append("valueA",  _taxCache.adj(0));
  params.append("valueB",  _taxCache.adj(1));
  params.append("valueC",  _taxCache.adj(2));
  params.append("pctA",    _taxCache.adjPct(0));
  params.append("pctB",    _taxCache.adjPct(1));
  params.append("pctC",    _taxCache.adjPct(2));
  
  if (newdlg.set(params) == NoError && newdlg.exec())
  {
    _taxCache.setAdjId(newdlg.tax());
    _taxCache.setAdj(newdlg.amountA(), newdlg.amountB(), newdlg.amountC());
    _taxCache.setAdjPct(newdlg.pctA(), newdlg.pctB(), newdlg.pctC());
    sRecalc();
  }
 }
}

void taxBreakdown::sFreightTaxDetail()
{
  taxDetail newdlg(this, "", true);
  ParameterList params;

   params.append("curr_id", _taxcurrency->id());
   params.append("date",    _freight->effective());
   params.append("subtotal",CurrDisplay::convert(_freight->id(),
						_taxcurrency->id(),
						_freight->localValue(),
						_freight->effective()));

  if (_ordertype == "S" || _ordertype == "Q")
  {
   params.append("taxzone_id",  _taxzone->id());

   if (_taxCache.freightType() > 0)
       params.append("taxtype_id",  _taxCache.freightType());
   else
   {
     q.exec("SELECT getfreighttaxtypeid() as taxtype;");
	 if(q.first())
		params.append("taxtype_id", q.value("taxtype").toInt());  
   }
   params.append("taxtype_id",  _taxCache.freightType());
  
   params.append("date",    _freight->effective());
  
   params.append("readOnly");
   if (newdlg.set(params) == NoError) 
	   newdlg.exec();
     
 }
 else if (_ordertype == "I" || _ordertype == "B")
 {
   if (_taxCache.freightType() > 0)
       params.append("taxtype_id",  _taxCache.freightType());
   else
   {
     q.exec("SELECT getfreighttaxtypeid() as taxtype;");
	 if(q.first())
		params.append("taxtype_id", q.value("taxtype").toInt());  
   }
   params.append("order_type", _ordertype);
   params.append("order_id", _orderid);
   params.append("display_type", "F");
   params.append("readOnly");
   if (newdlg.set(params) == NoError) 
	   newdlg.exec();  
 }
 else 
 {	 
  params.append("tax_id",  _taxCache.freightId());
  params.append("valueA",  _taxCache.freight(0));
  params.append("valueB",  _taxCache.freight(1));
  params.append("valueC",  _taxCache.freight(2));
  params.append("pctA",    _taxCache.freightPct(0));
  params.append("pctB",    _taxCache.freightPct(1));
  params.append("pctC",    _taxCache.freightPct(2));
 
  // s/o and quote don't have a place to store changes
  if (cView == _mode)
    params.append("readOnly");

  if (newdlg.set(params) == NoError && newdlg.exec())
  {
    _taxCache.setFreightId(newdlg.tax());
    _taxCache.setFreight(newdlg.amountA(), newdlg.amountB(), newdlg.amountC());
    _taxCache.setFreightPct(newdlg.pctA(), newdlg.pctB(), newdlg.pctC());
    sRecalc();
  }
}
}

void taxBreakdown::sLineTaxDetail()
{
  taxDetail newdlg(this, "", true);
  ParameterList params;

  if (_ordertype == "S" || _ordertype == "Q" || _ordertype == "I" || _ordertype == "B")
  {
    params.append("order_type", _ordertype);
    params.append("order_id", _orderid);
    params.append("display_type", "L");
  }
  else
  {
    params.append("valueA",  _taxCache.line(0));
    params.append("valueB",  _taxCache.line(1));
    params.append("valueC",  _taxCache.line(2));
  } 

  params.append("curr_id", _taxcurrency->id());
  params.append("date",    _line->effective());
  params.append("subtotal",CurrDisplay::convert(_line->id(),
						_taxcurrency->id(),
						_line->localValue(),
						_line->effective()));
  params.append("readOnly");

  if (newdlg.set(params) == NoError)
    newdlg.exec();
}

void taxBreakdown::sTotalTaxDetail()
{
  taxDetail newdlg(this, "", true);
  ParameterList params;
 
 if (_ordertype == "S" || _ordertype == "Q" || _ordertype == "I" || _ordertype == "B")
 {
   params.append("order_type", _ordertype);
   params.append("order_id", _orderid);
   params.append("display_type", "T");
 }
 else
 {
  params.append("valueA",  _taxCache.total(0));
  params.append("valueB",  _taxCache.total(1));
  params.append("valueC",  _taxCache.total(2));
 }

  params.append("curr_id", _taxcurrency->id());
  params.append("date",    _total->effective());
  params.append("subtotal",CurrDisplay::convert(_pretax->id(),
						_taxcurrency->id(),
						_pretax->localValue(),
						_pretax->effective()));
  params.append("readOnly");

  if (newdlg.set(params) == NoError)
    newdlg.exec();
}

double taxBreakdown::a()
{
  return _taxCache.total(0);
}

double taxBreakdown::b()
{
  return _taxCache.total(1);
}

double taxBreakdown::c()
{
  return _taxCache.total(2);
}

SetResponse taxBreakdown::sRecalc()
{
  _lineTax->setLocalValue(_taxCache.line());
  _freightTax->setLocalValue(_taxCache.freight());
  _adjTax->setLocalValue(_taxCache.adj());

  _totalTax->setLocalValue(_taxCache.total());

  _total->setLocalValue(_pretax->localValue() +
			CurrDisplay::convert(_totalTax->id(),
					     _total->id(),
					     _totalTax->localValue(),
					     _totalTax->effective())); 
  return NoError;
}

void taxBreakdown::sPopulateAdj()
{

  ParameterList params;
  if (_ordertype == "I")
    params.append("invchead_id", _orderid);
  else if (_ordertype == "B")
    params.append("cobmisc_id", _orderid);
  else if (_ordertype == "CM")
    params.append("cmhead_id", _orderid);
  else if (_ordertype == "TO")
    params.append("tohead_id", _orderid);

  QString sql =
	  "<? if exists(\"invchead_id\") ?>"
    	" SELECT  SUM(taxhist_tax) AS adjtaxamt "
        " FROM invchead LEFT OUTER JOIN invcheadtax ON (taxhist_parent_id = invchead_id AND "
		" taxhist_taxtype_id = getadjustmenttaxtypeid()) "
        " WHERE invchead_id=<? value(\"invchead_id\") ?>;"
      "<? elseif exists(\"cobmisc_id\") ?>"
       " SELECT  SUM(taxhist_tax) AS adjtaxamt "
        " FROM cobmisc LEFT OUTER JOIN cobmisctax ON (taxhist_parent_id = cobmisc_id AND "
		" taxhist_taxtype_id = getadjustmenttaxtypeid()) "
        " WHERE cobmisc_id=<? value(\"cobmisc_id\") ?>;"
      "<? endif ?>";

  MetaSQLQuery mql(sql);
  q = mql.toQuery(params);
  if (q.first())
  {
    _taxCache.setAdj(q.value("adjtaxamt").toDouble(), 0, 0); 
	sRecalc();
  }
}
