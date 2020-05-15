#pragma once

const char styleSheetLineEditNormal[500] = "QLineEdit {\
  background-color: #ffffff;\
  padding-top: 2px;\
  padding-bottom: 2px;\
  padding-left: 4px;\
  padding-right: 4px;\
  border-style: solid;\
  border: 1px solid rgb(122, 122, 122);\
}\
QLineEdit:disabled {\
  background-color: #19232D;\
  color: #787878;\
}\
QLineEdit:focus {\
  border: 1px solid rgb(0, 116, 207);\
}\
QLineEdit:hover {\
  border: 1px solid rgb(0, 0, 0);\
  widget-animation-duration: 10000;\
}\
QLineEdit:selected {\
  background-color: rgb(0, 116, 207);\
  color: #32414B;\
}";

const char styleSheetLineEditError[500] = "QLineEdit {\
  background-color: #ffffff;\
  padding-top: 2px;\
  padding-bottom: 2px;\
  padding-left: 4px;\
  padding-right: 4px;\
  border-style: solid;\
  border: 1px solid rgb(229, 20, 0);\
}\
QLineEdit:disabled {\
  background-color: #19232D;\
  color: #787878;\
}\
QLineEdit:focus {\
  border: 1px solid rgb(200, 100, 0);\
}\
QLineEdit:hover {\
  border: 1px solid rgb(200, 20, 0);\
}\
QLineEdit:selected {\
  background-color: rgb(200, 100, 0);\
  color: #32414B;\
}";
