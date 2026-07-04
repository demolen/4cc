/*
4coder_code_index_listers.cpp - Listers for exploring the contents of the code index.
*/

// TOP

struct Tiny_Jump{
    Buffer_ID buffer;
    i64 pos;
};

function View_ID
get_or_open_other_panel(Application_Links *app, View_ID active_view){
    View_ID target_view = get_next_view_looped_primary_panels(app, active_view, Access_Always);
    if (target_view == active_view){
        target_view = open_view(app, active_view, ViewSplit_Right);
        if (target_view != 0){
            new_view_settings(app, target_view);
            Buffer_ID buffer = view_get_buffer(app, active_view, Access_Always);
            view_set_buffer(app, target_view, buffer, 0);
        }
    }
    return(target_view);
}

CUSTOM_UI_COMMAND_SIG(jump_to_definition)
CUSTOM_DOC("List all definitions in the code index and jump to one chosen by the user.")
{
    char *query = "Definition:";
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);
    lister_set_default_handlers(lister);
    
    code_index_lock();
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_Always)){
        Code_Index_File *file = code_index_get_file(buffer);
        if (file != 0){
            for (i32 i = 0; i < file->note_array.count; i += 1){
                Code_Index_Note *note = file->note_array.ptrs[i];
                Tiny_Jump *jump = push_array(scratch, Tiny_Jump, 1);
                jump->buffer = buffer;
                jump->pos = note->pos.first;
                
                String_Const_u8 sort = {};
                switch (note->note_kind){
                    case CodeIndexNote_Type:
                    {
                        sort = string_u8_litexpr("type");
                    }break;
                    case CodeIndexNote_Function:
                    {
                        sort = string_u8_litexpr("function");
                    }break;
                    case CodeIndexNote_Macro:
                    {
                        sort = string_u8_litexpr("macro");
                    }break;
                    case CodeIndexNote_Constant:
                    {
                        sort = string_u8_litexpr("constant");
                    }break;
                }
                lister_add_item(lister, note->text, sort, jump, 0);
            }
        }
    }
    code_index_unlock();
    
    Lister_Result l_result = run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0){
        View_ID view = get_this_ctx_view(app, Access_Always);
        point_stack_push_view_cursor(app, view);
        jump_to_location(app, view, result.buffer, result.pos);
    }
}

function b32
jump_to_definition_at_cursor__inner(Application_Links *app, View_ID view, String_Const_u8 query){
    b32 result = false;
    code_index_lock();
    for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
         buffer != 0;
         buffer = get_buffer_next(app, buffer, Access_Always)){
        Code_Index_File *file = code_index_get_file(buffer);
        if (file != 0){
            for (i32 i = 0; i < file->note_array.count; i += 1){
                Code_Index_Note *note = file->note_array.ptrs[i];
                if (string_match(note->text, query)){
                    point_stack_push_view_cursor(app, view);
                    jump_to_location(app, view, buffer, note->pos.first);
                    result = true;
                    goto done;
                }
            }
        }
    }
    done:;
    code_index_unlock();
    return(result);
}

CUSTOM_UI_COMMAND_SIG(jump_to_definition_at_cursor)
CUSTOM_DOC("Jump to the first definition in the code index matching an identifier at the cursor")
{
    View_ID view = get_active_view(app, Access_Visible);
    
    if (view != 0){
        Scratch_Block scratch(app);
        String_Const_u8 query = push_token_or_word_under_active_cursor(app, scratch);
        jump_to_definition_at_cursor__inner(app, view, query);
    }
}

CUSTOM_UI_COMMAND_SIG(jump_to_definition_at_cursor_other_panel)
CUSTOM_DOC("Jump to the first definition in the code index matching an identifier at the cursor in the other panel.")
{
    View_ID active_view = get_active_view(app, Access_Visible);

    if (active_view != 0){
        Scratch_Block scratch(app);
        String_Const_u8 query = push_token_or_word_under_active_cursor(app, scratch);
        View_ID target_view = get_or_open_other_panel(app, active_view);
        if (target_view != 0){
            jump_to_definition_at_cursor__inner(app, target_view, query);
        }
    }
}

// BOTTOM
