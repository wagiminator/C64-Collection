#
# FreeBSD ports collection makefile for:	opencbm
#
# Date created:		Aug 04, 2012
#
# Written by:		Uffe Jakobsen <microtop@starion.dk>
#

PORTNAME=	opencbm
PORTVERSION=	0.4.99.95
CATEGORIES=	archivers
MASTER_SITES=	SF

LATEST_LINK=    ${PORTNAME}

COMMENT=	Commodore CBM serial IEC/IEEE-488 bus utilities
MAINTAINER=	uffe@uffe.org

LICENSE=	GPLv2
LICENSE_FILE=	${WRKSRC}/COPYING

WRKSRC=		${WRKDIR}/${DISTNAME}/${PORTNAME}

#USE_ZIP=	yes
USE_GZIP=	yes
#USE_BZIP2=	yes

USE_GMAKE=	yes

MAKE_JOBS_UNSAFE=	yes
MAKEFILE=	 LINUX/Makefile

EXTRACT_DEPENDS=	unzip:${PORTSDIR}/archivers/unzip
BUILD_DEPENDS=	cc65:${PORTSDIR}/devel/cc65

INSTALL_TARGET=	install-all

.include <bsd.port.mk>

