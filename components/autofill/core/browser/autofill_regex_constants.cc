// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains UTF8 strings that we want as char arrays.  To avoid
// different compilers, we use a script to convert the UTF8 strings into
// numeric literals (\x##).

#include "components/autofill/core/browser/autofill_regex_constants.h"

namespace autofill {

/////////////////////////////////////////////////////////////////////////////
// address_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kAttentionIgnoredRe[] = "attention|attn";
const char kRegionIgnoredRe[] =
    "province|region|other"
    "|provincia"  // es
    "|bairro|suburb";  // pt-BR, pt-PT
const char kAddressNameIgnoredRe[] = "address.*nickname|address.*label";
const char kCompanyRe[] =
    "company|business|organization|organisation"
    "|firma|firmenname"  // de-DE
    "|empresa"  // es
    "|societe|soci\xc3\xa9t\xc3\xa9"  // fr-FR
    "|ragione.?sociale"  // it-IT
    "|\xe4\xbc\x9a\xe7\xa4\xbe"  // ja-JP
    "|\xd0\xbd\xd0\xb0\xd0\xb7\xd0\xb2\xd0\xb0\xd0\xbd\xd0\xb8\xd0\xb5.?\xd0\xba\xd0\xbe\xd0\xbc\xd0\xbf\xd0\xb0\xd0\xbd\xd0\xb8\xd0\xb8"  // ru
    "|\xe5\x8d\x95\xe4\xbd\x8d|\xe5\x85\xac\xe5\x8f\xb8"  // zh-CN
    "|\xed\x9a\x8c\xec\x82\xac|\xec\xa7\x81\xec\x9e\xa5";  // ko-KR
const char kAddressLine1Re[] =
    "address.*line|address1|addr1|street"
    "|strasse|stra\xc3\x9f""e|hausnummer|housenumber"  // de-DE
    "|house.?name"  // en-GB
    "|direccion|direcci\xc3\xb3n"  // es
    "|adresse"  // fr-FR
    "|indirizzo"  // it-IT
    "|\xe4\xbd\x8f\xe6\x89\x80""1"  // ja-JP
    "|morada|endere\xc3\xa7o"  // pt-BR, pt-PT
    "|\xd0\x90\xd0\xb4\xd1\x80\xd0\xb5\xd1\x81"  // ru
    "|\xe5\x9c\xb0\xe5\x9d\x80"  // zh-CN
    "|\xec\xa3\xbc\xec\x86\x8c.?1";  // ko-KR
const char kAddressLine1LabelRe[] =
    "address"
    "|adresse"  // fr-FR
    "|indirizzo"  // it-IT
    "|\xe4\xbd\x8f\xe6\x89\x80"  // ja-JP
    "|\xe5\x9c\xb0\xe5\x9d\x80"  // zh-CN
    "|\xec\xa3\xbc\xec\x86\x8c";  // ko-KR
const char kAddressLine2Re[] =
    "address.*line2|address2|addr2|street|suite|unit"
    "|adresszusatz|erg\xc3\xa4nzende.?angaben"  // de-DE
    "|direccion2|colonia|adicional"  // es
    "|addresssuppl|complementnom|appartement"  // fr-FR
    "|indirizzo2"  // it-IT
    "|\xe4\xbd\x8f\xe6\x89\x80""2"  // ja-JP
    "|complemento|addrcomplement"  // pt-BR, pt-PT
    "|\xd0\xa3\xd0\xbb\xd0\xb8\xd1\x86\xd0\xb0"  // ru
    "|\xe5\x9c\xb0\xe5\x9d\x80""2"  // zh-CN
    "|\xec\xa3\xbc\xec\x86\x8c.?2";  // ko-KR
const char kAddressLine2LabelRe[] =
    "address"
    "|adresse"  // fr-FR
    "|indirizzo"  // it-IT
    "|\xe5\x9c\xb0\xe5\x9d\x80"  // zh-CN
    "|\xec\xa3\xbc\xec\x86\x8c";  // ko-KR
const char kAddressLinesExtraRe[] =
    "address.*line[3-9]|address[3-9]|addr[3-9]|street|line[3-9]"
    "|municipio"  // es
    "|batiment|residence"  // fr-FR
    "|indirizzo[3-9]";  // it-IT
const char kAddressLookupRe[] =
    "lookup";
const char kCountryRe[] =
    "country|countries|location"
    "|pa\xc3\xads|pais"  // es
    "|\xe5\x9b\xbd"  // ja-JP
    "|\xe5\x9b\xbd\xe5\xae\xb6"  // zh-CN
    "|\xea\xb5\xad\xea\xb0\x80|\xeb\x82\x98\xeb\x9d\xbc";  // ko-KR
const char kCountryLocationRe[] =
    "location";
const char kZipCodeRe[] =
    "zip|postal|post.*code|pcode"
    "|pin.?code"  // en-IN
    "|postleitzahl"  // de-DE
    "|\\bcp\\b"  // es
    "|\\bcdp\\b"  // fr-FR
    "|\\bcap\\b"  // it-IT
    "|\xe9\x83\xb5\xe4\xbe\xbf\xe7\x95\xaa\xe5\x8f\xb7"  // ja-JP
    "|codigo|codpos|\\bcep\\b"  // pt-BR, pt-PT
    "|\xd0\x9f\xd0\xbe\xd1\x87\xd1\x82\xd0\xbe\xd0\xb2\xd1\x8b\xd0\xb9.?\xd0\x98\xd0\xbd\xd0\xb4\xd0\xb5\xd0\xba\xd1\x81"  // ru
    "|\xe9\x82\xae\xe6\x94\xbf\xe7\xbc\x96\xe7\xa0\x81|\xe9\x82\xae\xe7\xbc\x96"  // zh-CN
    "|\xe9\x83\xb5\xe9\x81\x9e\xe5\x8d\x80\xe8\x99\x9f"  // zh-TW
    "|\xec\x9a\xb0\xed\x8e\xb8.?\xeb\xb2\x88\xed\x98\xb8";  // ko-KR
const char kZip4Re[] =
    "zip|^-$|post2"
    "|codpos2";  // pt-BR, pt-PT
const char kCityRe[] =
    "city|town"
    "|\\bort\\b|stadt"  // de-DE
    "|suburb"  // en-AU
    "|ciudad|provincia|localidad|poblacion"  // es
    "|ville|commune"  // fr-FR
    "|localita"  // it-IT
    "|\xe5\xb8\x82\xe5\x8c\xba\xe7\x94\xba\xe6\x9d\x91"  // ja-JP
    "|cidade"  // pt-BR, pt-PT
    "|\xd0\x93\xd0\xbe\xd1\x80\xd0\xbe\xd0\xb4"  // ru
    "|\xe5\xb8\x82"  // zh-CN
    "|\xe5\x88\x86\xe5\x8d\x80"  // zh-TW
    "|^\xec\x8b\x9c[^\xeb\x8f\x84\xc2\xb7\xe3\x83\xbb]|\xec\x8b\x9c[\xc2\xb7\xe3\x83\xbb]?\xea\xb5\xb0[\xc2\xb7\xe3\x83\xbb]?\xea\xb5\xac";  // ko-KR
const char kStateRe[] =
    "(?<!united )state|county|region|province"
    "|land"  // de-DE
    "|county|principality"  // en-UK
    "|\xe9\x83\xbd\xe9\x81\x93\xe5\xba\x9c\xe7\x9c\x8c"  // ja-JP
    "|estado|provincia"  // pt-BR, pt-PT
    "|\xd0\xbe\xd0\xb1\xd0\xbb\xd0\xb0\xd1\x81\xd1\x82\xd1\x8c"  // ru
    "|\xe7\x9c\x81"  // zh-CN
    "|\xe5\x9c\xb0\xe5\x8d\x80"  // zh-TW
    "|^\xec\x8b\x9c[\xc2\xb7\xe3\x83\xbb]?\xeb\x8f\x84";  // ko-KR

/////////////////////////////////////////////////////////////////////////////
// credit_card_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kNameOnCardRe[] =
    "card.?holder|name.*\\bon\\b.*card|cc.?name|cc.?full.?name|owner"
    "|karteninhaber"  // de-DE
    "|nombre.*tarjeta"  // es
    "|nom.*carte"  // fr-FR
    "|nome.*cart"  // it-IT
    "|\xe5\x90\x8d\xe5\x89\x8d"  // ja-JP
    "|\xd0\x98\xd0\xbc\xd1\x8f.*\xd0\xba\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b"  // ru
    "|\xe4\xbf\xa1\xe7\x94\xa8\xe5\x8d\xa1\xe5\xbc\x80\xe6\x88\xb7\xe5\x90\x8d|\xe5\xbc\x80\xe6\x88\xb7\xe5\x90\x8d|\xe6\x8c\x81\xe5\x8d\xa1\xe4\xba\xba\xe5\xa7\x93\xe5\x90\x8d"  // zh-CN
    "|\xe6\x8c\x81\xe5\x8d\xa1\xe4\xba\xba\xe5\xa7\x93\xe5\x90\x8d";  // zh-TW
const char kNameOnCardContextualRe[] =
    "name";
const char kCardNumberRe[] =
    "card.?number|card.?#|card.?no|cc.?num|acct.?num"
    "|nummer"  // de-DE
    "|credito|numero|n\xc3\xbamero"  // es
    "|num\xc3\xa9ro"  // fr-FR
    "|\xe3\x82\xab\xe3\x83\xbc\xe3\x83\x89\xe7\x95\xaa\xe5\x8f\xb7"  // ja-JP
    "|\xd0\x9d\xd0\xbe\xd0\xbc\xd0\xb5\xd1\x80.*\xd0\xba\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b"  // ru
    "|\xe4\xbf\xa1\xe7\x94\xa8\xe5\x8d\xa1\xe5\x8f\xb7|\xe4\xbf\xa1\xe7\x94\xa8\xe5\x8d\xa1\xe5\x8f\xb7\xe7\xa0\x81"  // zh-CN
    "|\xe4\xbf\xa1\xe7\x94\xa8\xe5\x8d\xa1\xe5\x8d\xa1\xe8\x99\x9f"  // zh-TW
    "|\xec\xb9\xb4\xeb\x93\x9c";  // ko-KR
const char kCardCvcRe[] =
    "verification|card identification|security code|cvn|cvv|cvc|csc|\\bcid\\b";
const char kCardTypeRe[] =
    "card.?type|cc.?type|payment.?method";

// "Expiration date" is the most common label here, but some pages have
// "Expires", "exp. date" or "exp. month" and "exp. year".  We also look
// for the field names ccmonth and ccyear, which appear on at least 4 of
// our test pages.

// On at least one page (The China Shop2.html) we find only the labels
// "month" and "year".  So for now we match these words directly; we'll
// see if this turns out to be too general.

// Toolbar Bug 51451: indeed, simply matching "month" is too general for
//   https://rps.fidelity.com/ftgw/rps/RtlCust/CreatePIN/Init.
// Instead, we match only words beginning with "month".
const char kExpirationMonthRe[] =
    "expir|exp.*mo|exp.*date|ccmonth|cardmonth"
    "|gueltig|g\xc3\xbcltig|monat"  // de-DE
    "|fecha"  // es
    "|date.*exp"  // fr-FR
    "|scadenza"  // it-IT
    "|\xe6\x9c\x89\xe5\x8a\xb9\xe6\x9c\x9f\xe9\x99\x90"  // ja-JP
    "|validade"  // pt-BR, pt-PT
    "|\xd0\xa1\xd1\x80\xd0\xbe\xd0\xba \xd0\xb4\xd0\xb5\xd0\xb9\xd1\x81\xd1\x82\xd0\xb2\xd0\xb8\xd1\x8f \xd0\xba\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b"  // ru
    "|\xe6\x9c\x88";  // zh-CN
const char kExpirationYearRe[] =
    "exp|^/|year"
    "|ablaufdatum|gueltig|g\xc3\xbcltig|yahr"  // de-DE
    "|fecha"  // es
    "|scadenza"  // it-IT
    "|\xe6\x9c\x89\xe5\x8a\xb9\xe6\x9c\x9f\xe9\x99\x90"  // ja-JP
    "|validade"  // pt-BR, pt-PT
    "|\xd0\xa1\xd1\x80\xd0\xbe\xd0\xba \xd0\xb4\xd0\xb5\xd0\xb9\xd1\x81\xd1\x82\xd0\xb2\xd0\xb8\xd1\x8f \xd0\xba\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b"  // ru
    "|\xe5\xb9\xb4|\xe6\x9c\x89\xe6\x95\x88\xe6\x9c\x9f";  // zh-CN

// This regex is a little bit nasty, but it is simply requiring exactly two
// adjacent y's.
const char kExpirationDate2DigitYearRe[] =
    "(?:exp.*date[^y\\n\\r]*|mm\\s*[-/]?\\s*)yy(?:[^y]|$)";
// Used to match a expiration date field with a four digit year.
// Same requirements as |kExpirationDate2DigitYearRe| except:
//  - Exactly four adjacent y's.
// Tested in components/autofill/core/common/autofill_regexes_unittest.cc
const char kExpirationDate4DigitYearRe[] =
    "(?:exp.*date[^y\\n\\r]*|mm\\s*[-/]?\\s*)yyyy(?:[^y]|$)";
// Used to match expiration date fields that do not specify a year length.
const char kExpirationDateRe[] =
    "expir|exp.*date"
    "|gueltig|g\xc3\xbcltig"  // de-DE
    "|fecha"  // es
    "|date.*exp"  // fr-FR
    "|scadenza"  // it-IT
    "|\xe6\x9c\x89\xe5\x8a\xb9\xe6\x9c\x9f\xe9\x99\x90"  // ja-JP
    "|validade"  // pt-BR, pt-PT
    "|\xd0\xa1\xd1\x80\xd0\xbe\xd0\xba \xd0\xb4\xd0\xb5\xd0\xb9\xd1\x81\xd1\x82\xd0\xb2\xd0\xb8\xd1\x8f \xd0\xba\xd0\xb0\xd1\x80\xd1\x82\xd1\x8b";  // ru
const char kCardIgnoredRe[] =
    "^card";
const char kGiftCardRe[] =
    "gift.?card";
const char kDebitGiftCardRe[] =
    "(?:visa|mastercard|discover|amex|american express).*gift.?card";
const char kDebitCardRe[] =
    "debit.*card";


/////////////////////////////////////////////////////////////////////////////
// email_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kEmailRe[] =
    "e.?mail"
    "|courriel"  // fr
    "|\xe3\x83\xa1\xe3\x83\xbc\xe3\x83\xab\xe3\x82\xa2\xe3\x83\x89\xe3\x83\xac\xe3\x82\xb9"  // ja-JP
    "|\xd0\xad\xd0\xbb\xd0\xb5\xd0\xba\xd1\x82\xd1\x80\xd0\xbe\xd0\xbd\xd0\xbd\xd0\xbe\xd0\xb9.?\xd0\x9f\xd0\xbe\xd1\x87\xd1\x82\xd1\x8b"  // ru
    "|\xe9\x82\xae\xe4\xbb\xb6|\xe9\x82\xae\xe7\xae\xb1"  // zh-CN
    "|\xe9\x9b\xbb\xe9\x83\xb5\xe5\x9c\xb0\xe5\x9d\x80"  // zh-TW
    "|(\xec\x9d\xb4\xeb\xa9\x94\xec\x9d\xbc|\xec\xa0\x84\xec\x9e\x90.?\xec\x9a\xb0\xed\x8e\xb8|[Ee]-?mail)(.?\xec\xa3\xbc\xec\x86\x8c)?";  // ko-KR


/////////////////////////////////////////////////////////////////////////////
// name_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kNameIgnoredRe[] =
    "user.?name|user.?id|nickname|maiden name|title|prefix|suffix"
    "|vollst\xc3\xa4ndiger.?name"  // de-DE
    "|\xe7\x94\xa8\xe6\x88\xb7\xe5\x90\x8d"  // zh-CN
    "|(\xec\x82\xac\xec\x9a\xa9\xec\x9e\x90.?)?\xec\x95\x84\xec\x9d\xb4\xeb\x94\x94|\xec\x82\xac\xec\x9a\xa9\xec\x9e\x90.?ID";  // ko-KR
const char kNameRe[] =
    "^name|full.?name|your.?name|customer.?name|firstandlastname|bill.?name"
        "|ship.?name"
    "|nombre.*y.*apellidos"  // es
    "|^nom"  // fr-FR
    "|\xe3\x81\x8a\xe5\x90\x8d\xe5\x89\x8d|\xe6\xb0\x8f\xe5\x90\x8d"  // ja-JP
    "|^nome"  // pt-BR, pt-PT
    "|\xe5\xa7\x93\xe5\x90\x8d"  // zh-CN
    "|\xec\x84\xb1\xeb\xaa\x85";  // ko-KR
const char kNameSpecificRe[] =
    "^name"
    "|^nom"  // fr-FR
    "|^nome";  // pt-BR, pt-PT
const char kFirstNameRe[] =
    "first.*name|initials|fname|first$"
    "|vorname"  // de-DE
    "|nombre"  // es
    "|forename|pr\xc3\xa9nom|prenom"  // fr-FR
    "|\xe5\x90\x8d"  // ja-JP
    "|nome"  // pt-BR, pt-PT
    "|\xd0\x98\xd0\xbc\xd1\x8f"  // ru
    "|\xec\x9d\xb4\xeb\xa6\x84";  // ko-KR
const char kMiddleInitialRe[] = "middle.*initial|m\\.i\\.|mi$|\\bmi\\b";
const char kMiddleNameRe[] =
    "middle.*name|mname|middle$"
    "|apellido.?materno|lastlastname";  // es
const char kLastNameRe[] =
    "last.*name|lname|surname|last$|secondname"
    "|nachname"  // de-DE
    "|apellido"  // es
    "|famille|^nom"  // fr-FR
    "|cognome"  // it-IT
    "|\xe5\xa7\x93"  // ja-JP
    "|morada|apelidos|surename|sobrenome"  // pt-BR, pt-PT
    "|\xd0\xa4\xd0\xb0\xd0\xbc\xd0\xb8\xd0\xbb\xd0\xb8\xd1\x8f"  // ru
    "|\xec\x84\xb1[^\xeb\xaa\x85]?";  // ko-KR

/////////////////////////////////////////////////////////////////////////////
// phone_field.cc
/////////////////////////////////////////////////////////////////////////////
const char kPhoneRe[] =
    "phone|mobile"
    "|telefonnummer"  // de-DE
    "|telefono|tel\xc3\xa9""fono"  // es
    "|telfixe"  // fr-FR
    "|\xe9\x9b\xbb\xe8\xa9\xb1"  // ja-JP
    "|telefone|telemovel"  // pt-BR, pt-PT
    "|\xd1\x82\xd0\xb5\xd0\xbb\xd0\xb5\xd1\x84\xd0\xbe\xd0\xbd"  // ru
    "|\xe7\x94\xb5\xe8\xaf\x9d"  // zh-CN
    "|(\xec\xa0\x84\xed\x99\x94|\xed\x95\xb8\xeb\x93\x9c\xed\x8f\xb0|\xed\x9c\xb4\xeb\x8c\x80\xed\x8f\xb0|\xed\x9c\xb4\xeb\x8c\x80\xec\xa0\x84\xed\x99\x94)(.?\xeb\xb2\x88\xed\x98\xb8)?";  // ko-KR
const char kCountryCodeRe[] =
    "country.*code|ccode|_cc";
const char kAreaCodeNotextRe[] =
    "^\\($";
const char kAreaCodeRe[] =
    "area.*code|acode|area"
    "|\xec\xa7\x80\xec\x97\xad.?\xeb\xb2\x88\xed\x98\xb8";  // ko-KR
const char kPhonePrefixSeparatorRe[] =
    "^-$|^\\)$";
const char kPhoneSuffixSeparatorRe[] =
    "^-$";
const char kPhonePrefixRe[] =
    "prefix|exchange"
    "|preselection"  // fr-FR
    "|ddd";  // pt-BR, pt-PT
const char kPhoneSuffixRe[] =
    "suffix";
const char kPhoneExtensionRe[] =
    "\\bext|ext\\b|extension"
    "|ramal";  // pt-BR, pt-PT

}  // namespace autofill
