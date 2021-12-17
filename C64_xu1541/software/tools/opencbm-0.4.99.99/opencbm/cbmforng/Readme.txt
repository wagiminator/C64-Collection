The next generation  CBMFormat  for OpenCBM

CBMForNG is a nearly complete rewrite (98%) of cbmformat that was
improved by Spiro Trikalitotis to do GAP size probing and a verify
round after the format.

  Base code:                            (C) 1994-2004 Joe Forster/STA
  Adaption to cbm4linux, progress bar:  (C) 1999      Michael Klein
  GAP probing and further improvements: (C) 2005      Spiro Trikaliotis
  All rewritten parts:                  (C) 2005,2006 Wolfgang Moser

Licenses:
  Host program: GNU General Public License
  6502 code:    modified BSD license (the 3 clause BSD license)


Some of the features implemented in the rewritten  6502  routine:

   * 5-byte GCR pattern pieces based track format creation scheme:
     This way the needed buffer size for the data block GCR pattern,
     the GCR header block as well as some format parameters could
     be reduced to only 141 bytes in sum. That buffer is stored to
     $0173...$01ff (second BAM for the 1571 and GCR conversion
     overflow area), so all remaining buffer can be used for code.

   * Host based data block GCR pattern pieces creation:
     Because the 30 byte data block GCR pattern pieces are created
     at the host side and transferred to the floppy together with
     the format parameters at lot of code could be saved (over 50
     bytes) within the floppy side. Furthermore this allows greater
     flexibility in creating custom format patterns.

   * Nybble based GAP creation:
     On using some sort of alternation scheme (based on the oddity
     of the sector number to write next), inter sector GAP sizes
     with an effective granularity of half a byte can be created.

   * Nybble based GAP probing algorithm:
     Spiro's probing algorithm was redesigned completely, so that
     all the GAP size calculation are nybble based too. This ensures
     that the track tail GAP never becomes greater than double the
     size of the formerly used inter sector GAP and never becomes
     less than one less than the inter sector GAP size.

   * Cushioning technique for applying probed results on belt-driven
     drives:
     To reduce the probability for the need to reformat a formerly
     formatted track especially on belt-driven drives, inter sector
     GAPs are not increased to the theoretically best value, so that

        interSectorGAP - 1 <= trackTailGAP <= 1.5 * interSectorGAP

     instead the new GAP is left half a byte less than the optimal
     value, which in sum leads to:

        interSectorGAP - 1 <= trackTailGAP <= 2 * interSectorGAP

   * 1571 fix for the SYNC test after probing:
     Since the 1571 disk drive seems not to be able to deserialize
     the bitsream correctly directly after it has been switched into
     read mode, some SYNCs are left over before the test is done
     that the track tail did not overwrite the beginning. The exact
     reason for this behaviour could not be cleared until now, but
     it heavily depends on the data block pattern that is _written_
     before. Possible causes are the PLL in the digital electronics
     of the 1571 in combination with the drive mechanic (R/W head
     parameters).

   * Heavily stretched timing conditions upon the Byte-Ready signal:
     For the function entry and exit of the 5 byte GCR pattern block
     writer and compare routines, the general timing restriction of
     26 cycles (worst case on read: 25) between two consecutive
     writes or reads is broken. Through careful design and timing
     compensations just in time, this potential problem was reverted
     into a principle, so that a completely valid format pattern is
     sequenced to disk.

   * Integration with the DOS ROM:
     To reduce the code size as most as possible, error handling as
     well as doing retries on errors is done by the DOS ROMs job
     processor without losing the ability to format extended disks
     (40 tracks) on drives that don't support more than 35 tracks.

Drawbacks against the former  CBMFormat:

   * The progress bar is not available anymore:
     Since the progress bar handshaking code was not compatible to
     the DOS based error handling and retry on error scheme, it had
     to be removed.

   * Header SYNCs are sometimes shorter than the standard:
     One side effect of the nybble based GAP creation routine is
     that the SYNC marks of the sector headers sometimes become only
     32 bits long instead of the standard length of 40 bits. This is
     definetly no problem, even for more sophisticated disk access
     routines (reading as well as writing). The SYNC marks of the
     data blocks as well as the GAPs between the end of the header
     and the data block SYNC always get the standard length, which
     is much more important for correctly working write operations
     and disk drive interoperability (PET floppies).


New, removed and currently hidden parameters for  cbmforng.exe:
   -v      enable verify
   -c      clear out track before format (write Killer track)
   -r n    number of retries, 1...63 (disk based parameter)

   -p      removed switch, was: progress bar

   -f n    format fill pattern, exclusive to -o;
           uncompleted, check sourcecode for details
   -b n    begin track to start the format process from
   -e n    end track, exclusive to -x

All other switches didn't change their functionaliy to formerly
known versions of cbmformat.


In the current development version of the next generation formatter
some sort of debug logging functionality is integrated. After the
format process was completed, all the track based probing parameters
are downloaded from the floppy drive and printed to screen. This
functionality will/may be removed in production versions of the next
generation formatter.


Wolfgang Moser, 2009-02-10
