#include "rpn_input_controller.h"
#include "app.h"
#include <assert.h>
#include <poincare_nodes.h>

using namespace Poincare;

namespace Rpn {

InputController::InputController(Responder * parentResponder, Stack * stack, StackController * stackController, ContentView * view) :
  ViewController(parentResponder),
  m_stack(stack),
  m_stackController(stackController),
  m_view(view)
{
}

View * InputController::view() {
  return m_view;
}

void InputController::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(inputView());
  inputView()->setEditing(true);
  m_stackController->reloadAndScroll();
}

bool InputController::handleEvent(Ion::Events::Event event) {
  return false;
}

bool InputController::textFieldShouldFinishEditing(TextField * view, Ion::Events::Event event) {
  return event == Ion::Events::EXE || event == Ion::Events::OK;
}

bool InputController::textFieldDidReceiveEvent(TextField * view, Ion::Events::Event event) {
  if (handleEventSpecial(event, view) || handleEventOperation(event, view)) {
    m_stackController->reloadAndScroll();
    return true;
  }

  return false;
}

bool InputController::textFieldDidFinishEditing(TextField * view, const char * text, Ion::Events::Event event) {
  if (pushInput()) {
    m_stackController->reloadAndScroll();
    return true;
  }
  else {
    return false;
  }
}

bool InputController::textFieldDidAbortEditing(TextField * view) {
  m_stackController->reloadAndScroll();
  //Container::activeApp()->setFirstResponder(m_view->mainView());
  return true;
}

bool InputController::textFieldDidHandleEvent(TextField * view, bool returnValue, bool textHasChanged) {
  return returnValue;
}

bool InputController::handleEventSpecial(Ion::Events::Event event, TextField * view) {
  const char *text = view->text();
  bool handled = true;
  I18n::Message r = I18n::Message::Default;

  if (event == Ion::Events::Up && !m_stackController->empty()) {
    view->setEditing(false);
    Container::activeApp()->setFirstResponder(m_stackController);
  }
  else if (event == Ion::Events::Backspace && *text == '\0') {
    (*m_stackController)(Stack::POP);
  }
  else if (event == Ion::Events::Clear) {
    if (view->isEditing() && *text) {
      view->setText("");
      view->setCursorLocation(view->text());
    }
    else {
      (*m_stackController)(Stack::CLEAR);
    }
  }
  else if (event == Ion::Events::Exp) {
    if (pushInput()) {
      r = (*m_stackController)(Stack::Exp, ((Rpn::App*) Container::activeApp())->localContext());
    }
  }
  else if (event == Ion::Events::Log) {
    if (pushInput()) {
      r = (*m_stackController)(Stack::CommonLogarithm, ((Rpn::App*) Container::activeApp())->localContext());
    }
  }
  else if (event == Ion::Events::Equal) {
    m_stackController->setApproximate(!m_stackController->approximate());
  }
  else if (event == Ion::Events::Square) {
    if (pushInput()) {
      r = (*m_stackController)(Stack::Square, ((Rpn::App*) Container::activeApp())->localContext());
    }
  }
  else if (event == Ion::Events::RightParenthesis) {
    (*m_stackController)(Stack::SWAP);
  }
  else if (event == Ion::Events::LeftParenthesis) {
    (*m_stackController)(Stack::ROT);
  }
  else if (event == Ion::Events::Ans) {
    r = (*m_stackController)(Stack::OVER);
  }
  else if ((event == Ion::Events::EXE || event == Ion::Events::OK) && *text == '\0') {
    r = (*m_stackController)(Stack::DUP);
  }
  else {
    handled = false;
  }

  if (handled) {
    inputView()->setText("");
    inputView()->setCursorLocation(inputView()->text());
  }
  if (r != I18n::Message::Default) {
    Container::activeApp()->displayWarning(r);
  }

  return handled;
}

struct Event2Type {
  Ion::Events::Event event;
  ExpressionNode::Type op;
} ;

constexpr static Event2Type events2types[] {
  { Ion::Events::Plus, ExpressionNode::Type::Addition },
  { Ion::Events::Minus, ExpressionNode::Type::Subtraction },
  { Ion::Events::Multiplication, ExpressionNode::Type::Multiplication },
  { Ion::Events::Division, ExpressionNode::Type::Division },

  { Ion::Events::Ln, ExpressionNode::Type::NaperianLogarithm },
  { Ion::Events::Power, ExpressionNode::Type::Power },

  { Ion::Events::Sine, ExpressionNode::Type::Sine },
  { Ion::Events::Cosine, ExpressionNode::Type::Cosine },
  { Ion::Events::Tangent, ExpressionNode::Type::Tangent },
  { Ion::Events::Arcsine, ExpressionNode::Type::ArcSine },
  { Ion::Events::Arccosine, ExpressionNode::Type::ArcCosine },
  { Ion::Events::Arctangent, ExpressionNode::Type::ArcTangent },

  { Ion::Events::Sqrt, ExpressionNode::Type::SquareRoot },
  { Ion::Events::Space, ExpressionNode::Type::Opposite },
} ;

bool InputController::handleEventOperation(Ion::Events::Event event, TextField * view) {
  for (size_t i = 0; i < sizeof(events2types)/sizeof(events2types[0]); i++) {
    if (events2types[i].event == event) {
      if (pushInput()) {
        auto r = (*m_stack)(events2types[i].op, ((Rpn::App*) Container::activeApp())->localContext());
        if (r != I18n::Message::Default) {
          Container::activeApp()->displayWarning(r);
        }
      }
      return true;
    }
  }

  return false;
}

bool InputController::pushInput() {
  const char *text = inputView()->draftTextBuffer();

  if (*text == '\0') {
    return true;
  }

  auto r = (*m_stack)(text, ((Rpn::App*) Container::activeApp())->localContext());
  if (r == I18n::Message::Default) {
    inputView()->setText("");
    inputView()->setCursorLocation(inputView()->text());
    return true;
  }
  else {
    Container::activeApp()->displayWarning(r);
    return false;
  }
}

void InputController::setText(const char *text) {
  inputView()->setEditing(true);
  inputView()->setText(text);
}

TextField* InputController::inputView() {
  return m_view->inputView();
}

}
