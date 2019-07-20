#include "include/getaccounthistory.hpp"

#include <panels/commands.hpp>
#include <panels/cli.hpp>

GetAccountHistory::GetAccountHistory(GWallet* gwallet)
{
   p_GWallet = gwallet;

   wxXmlResource::Get()->LoadAllFiles(p_GWallet->directory + wxT("/plugins/getters/resources"));
   InitWidgetsFromXRC((wxWindow *)p_GWallet);

   SetScrollRate(1,1);

   Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GetAccountHistory::OnOk));
   Connect(XRCID("name"), wxEVT_SEARCHCTRL_SEARCH_BTN, wxCommandEventHandler(GetAccountHistory::OnSearchAccount), NULL, this);

   name->SetValue(p_GWallet->strings.selected_account);
}

void GetAccountHistory::OnSearchAccount(wxCommandEvent& event)
{
   p_GWallet->DoSearchAccount(event.GetString(), *name);
}

void GetAccountHistory::OnOk(wxCommandEvent& WXUNUSED(event))
{
   const auto _name = name->GetValue().ToStdString();
   const auto _limit = limit->GetValue();

   vector<graphene::wallet::operation_detail> result;
   wxAny response;

   p_GWallet->panels.p_commands->Wait();

   try
   {
      result = p_GWallet->bitshares.wallet_api_ptr->get_account_history(_name, _limit);
      response = result;
   }
   catch(const fc::exception& e)
   {
      p_GWallet->OnError(this, _("Account not found"));
      name->SetFocus();
      return;
   }

   new GetAccountHistoryResponse(p_GWallet, response);

   if(cli->IsChecked())
   {
      auto command = "get_account_history " + _name + " " + to_string(_limit);
      p_GWallet->panels.p_cli->DoCommand(command);
   }
}

GetAccountHistoryResponse::GetAccountHistoryResponse(GWallet* gwallet, wxAny any_response)
{
   InitWidgetsFromXRC((wxWindow *)gwallet);

   response_grid->CreateGrid(0, 4);

   SetScrollRate(1,1);
   response_grid->ShowScrollbars(wxSHOW_SB_NEVER,wxSHOW_SB_NEVER);

   gwallet->panels.p_commands->DoGridProperties(response_grid);

   response_grid->SetColLabelValue(0, "ID");
   response_grid->SetColLabelValue(1, _("Date"));
   response_grid->SetColLabelValue(2, _("Block"));
   response_grid->SetColLabelValue(3, _("Text"));

   auto results = any_response.As<vector<graphene::wallet::operation_detail>>();

   auto z = 0;

   response_grid->BeginBatch();
   response_grid->ClearSelection();

   response_grid->AppendRows(results.size());
   for( auto& ah : results ) {

      const auto b = gwallet->bitshares.database_api->get_block_header(ah.op.block_num);
      FC_ASSERT(b);

      response_grid->SetCellValue(z, 0, fc::json::to_string(ah.op.id).substr(1, fc::json::to_string(ah.op.id).length() - 2));
      response_grid->SetCellValue(z, 1, b->timestamp.to_iso_string());
      response_grid->SetCellValue(z, 2, fc::to_string(ah.op.block_num));
      response_grid->SetCellValue(z, 3, ah.description);

      ++z;
   }
   response_grid->AutoSize();
   response_grid->ForceRefresh();
   response_grid->EndBatch();

   gwallet->panels.p_commands->notebook->AddPage(this, _("Account history response"), true);
}