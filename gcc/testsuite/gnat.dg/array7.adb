-- { dg-do compile }
-- { dg-options "-O -gnatp -fdump-tree-optimized" }

package body Array7 is

   function Get_Arr (Nbr : My_Range) return Arr_Acc is
   begin
      return new Arr (1 .. Nbr);
   end;

end Array7;

-- { dg-final { scan-tree-dump-not "MAX_EXPR" "optimized" } }
