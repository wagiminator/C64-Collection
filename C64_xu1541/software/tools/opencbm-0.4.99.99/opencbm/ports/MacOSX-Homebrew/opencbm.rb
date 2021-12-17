require 'formula'

class Opencbm < Formula
  homepage 'http://opencbm.sourceforge.net/'
  url 'http://downloads.sourceforge.net/project/cbm/cbm/0.4.99.95/opencbm-src-0.4.99.95.tgz'
  md5 'NA'

  def install
    system "make", "-C", "opencbm", "-f", "LINUX/Makefile", "PREFIX=#{prefix)", "install-all"
  end

end

