#pragma once

#include "standard_vis.hpp"

namespace online_visualization {

  // write-once history
  class History
  {
    typedef HIST_ENTRY EntryT;

   public:
    void AddEntry(const string& entry) {
      add_history(entry.c_str());
      ++size_;
      VERIFY(int(size_) == history_get_history_state()->length);
    }

    const char* operator[](size_t k) const {
      VERIFY(k < size_);
      EntryT** my_history = history_list();
      return my_history[k]->line;
    }

    void SetEntry(size_t k, const string& entry) const {
      VERIFY(k < size_);
      replace_history_entry(k, entry.c_str(), history_list()[k]->data);
    }

    size_t size() const {
      return size_;
    }

    const char* front() const {
      return this->operator[](0);
    }

    const char* back() const {
      return this->operator[](this->size() - 1);
    }

    static History& GetHistory() {
      static History hist;
      return hist;
    }

   private:
    size_t size_;

    History() : size_(0)
    {
    }
  };

}
