// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-06-05 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/impl/chord_composer.h>

static const char* kZeroWidthSpace = "\xe2\x80\x8b";

namespace rime {


ChordComposer::ChordComposer(Engine *engine) : Processor(engine),
                                               pass_thru_(false) {
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("chord_composer/alphabet", &alphabet_);
    algebra_.Load(config->GetList("chord_composer/algebra"));
  }
}

Processor::Result ChordComposer::ProcessKeyEvent(const KeyEvent &key_event) {
  if (pass_thru_)
    return kNoop;
  bool composing = !chord_.empty();
  if (key_event.shift() || key_event.ctrl() || key_event.alt()) {
    ClearChord();
    return composing ? kAccepted : kNoop;
  }
  bool is_key_up = key_event.release();
  int ch = key_event.keycode();
  if (alphabet_.find(ch) == std::string::npos) {
    ClearChord();
    return composing ? kAccepted : kNoop;
  }
  // in alphabet
  if (is_key_up) {
    if (pressed_.erase(ch) != 0 && pressed_.empty()) {
      FinishChord();
    }
  }
  else {  // key down
    pressed_.insert(ch);
    bool updated = chord_.insert(ch).second;
    if (updated)
      UpdateChord();
  }
  return kAccepted;
}

const std::string ChordComposer::SerializeChord() {
  std::string code;
  BOOST_FOREACH(char ch, alphabet_) {
    if (chord_.find(ch) != chord_.end())
      code.push_back(ch);
  }
  algebra_.Apply(&code);
  return code;
}

void ChordComposer::UpdateChord() {
  if (!engine_) return;
  Context* ctx = engine_->context();
  Composition* comp = ctx->composition();
  bool chord_exists = !comp->empty() && comp->back().HasTag("chord");
  bool chord_prompt = !comp->empty() && comp->back().HasTag("chord_prompt");
  if (chord_.empty()) {
    if (chord_exists) {
      ctx->ClearPreviousSegment();
    }
    else if (chord_prompt) {
      comp->back().prompt.clear();
      comp->back().tags.erase("chord_prompt");
    }
  }
  else {
    std::string code(SerializeChord());
    if (!chord_exists && !chord_prompt) {
      if (comp->empty()) {
        comp->Forward();
        ctx->PushInput(kZeroWidthSpace);
        comp->back().tags.insert("chord");
      }
      else {
        comp->back().tags.insert("chord_prompt");
      }
    }
    comp->back().prompt = code;
  }
}

void ChordComposer::FinishChord() {
  if (!engine_) return;
  std::string code(SerializeChord());
  ClearChord();
  
  KeySequence sequence;
  if (sequence.Parse(code)) {
    pass_thru_ = true;
    BOOST_FOREACH(const KeyEvent& ke, sequence) {
      engine_->ProcessKeyEvent(ke);
    }
    pass_thru_ = false;
  }
}

void ChordComposer::ClearChord() {
    pressed_.clear();
    chord_.clear();
    UpdateChord();
}

}  // namespace rime
