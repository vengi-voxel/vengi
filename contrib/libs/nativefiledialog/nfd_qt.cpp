/*
 Native File Dialog

 http://www.frogtoss.com/labs
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <QApplication>
#include <QFileDialog>
#include "nfd.h"
#include "nfd_common.h"

const char NOPATH_MSG[] = "The selected path is out of memory.";
const char DIALOG_CLOSED_MSG[] = "Dialog was closed.";
const char NOMEM_MSG[] = "Out of memory.";

static void AddFiltersToDialog( QFileDialog &dialog, const char *filterList )
{
    if ( !filterList )
    {
        return;
    }
    QStringList filterStringList;

    QStringList filters = QString( filterList ).split( ';' );
    QListIterator<QString> iter( filters );
    while ( iter.hasNext() )
    {
        const QString& filterEntry = iter.next();
        const QStringList& wildcards = filterEntry.split( ',' );
        QListIterator<QString> iterWildcards( wildcards );
        QString filterString;
        while ( iterWildcards.hasNext() )
        {
            filterString.append( "*." ).append(iterWildcards.next());
            if ( iterWildcards.hasNext() )
            {
                filterString.append( ' ' );
            }
        }
        filterStringList << filterString;
    }

    filterStringList << "Any files (*)";
    dialog.setNameFilters( filterStringList );
}

static nfdresult_t NFD_QTOpenDialog(QFileDialog::AcceptMode acceptMode, QFileDialog::FileMode fileMode,
        const nfdchar_t *prompt, const char *filterList, const nfdchar_t *defaultFilename,
        const nfdchar_t *defaultPath, nfdchar_t **outPath, nfdpathset_t *outPaths) {
    int argc = 0;
    QApplication app( argc, nullptr );

    QFileDialog dialog;
    dialog.setAcceptMode( acceptMode );
    dialog.setFileMode( fileMode );
    dialog.setWindowFlags( Qt::WindowStaysOnTopHint );
    dialog.setWindowModality( Qt::ApplicationModal );
    AddFiltersToDialog( dialog, filterList );

    if ( defaultPath )
    {
        dialog.setDirectory( defaultPath );
    }
    if ( defaultFilename )
    {
        dialog.selectFile( defaultFilename );
    }
    if ( prompt )
    {
        dialog.setLabelText( QFileDialog::LookIn, prompt );
    }

    nfdresult_t result = NFD_CANCEL;
    dialog.show();
    app.processEvents();
    dialog.activateWindow();
    app.processEvents();
    dialog.raise();

    for (;;) {
        app.processEvents();
        if (dialog.isHidden()) {
            NFDi_SetError(DIALOG_CLOSED_MSG);
            result = NFD_ERROR;
            break;
        }
        const int dialogResult = dialog.result();
        if ( dialogResult != 0 )
        {
            const QStringList& selectedFiles = dialog.selectedFiles();
            if ( selectedFiles.empty() )
            {
                result = NFD_CANCEL;
                break;
            }
            if ( outPath )
            {
                const QString& entry = selectedFiles.at( 0 );
                const size_t len = entry.size();
                const QByteArray& ba = entry.toLatin1();
                const char *cstr = ba.data();
                *outPath = (nfdchar_t*)NFDi_Malloc( len + 1 );
                memcpy( *outPath, cstr, len + 1 );
                if ( !*outPath )
                {
                    NFDi_SetError( NOPATH_MSG );
                    result = NFD_ERROR;
                }
                else
                {
                    result = NFD_OKAY;
                }
            }
            else if ( outPaths )
            {
                outPaths->count = (size_t) selectedFiles.size();
                outPaths->indices = (size_t *)NFDi_Malloc( sizeof( size_t ) * outPaths->count );
                if ( !outPaths->indices )
                {
                    NFDi_SetError( NOMEM_MSG );
                    result = NFD_ERROR;
                    break;
                }

                size_t bufSize = 0;
                QListIterator<QString> iter( selectedFiles );
                while ( iter.hasNext() )
                {
                    const QString& entry = iter.next();
                    bufSize += entry.size() + 1;
                }

                outPaths->buf = (nfdchar_t *)NFDi_Malloc( sizeof( nfdchar_t ) * bufSize );

                nfdchar_t *p_buf = outPaths->buf;
                iter = QListIterator<QString>( selectedFiles );
                int count = 0;
                while ( iter.hasNext() )
                {
                    const QString& entry = iter.next();
                    const QByteArray& ba = entry.toLatin1();
                    const char *cstr = ba.data();
                    nfdchar_t *path = (nfdchar_t*) (cstr);
                    size_t byteLen = entry.size() + 1;
                    memcpy( p_buf, path, byteLen );
                    const ptrdiff_t index = p_buf - outPaths->buf;
                    assert(index >= 0);
                    outPaths->indices[count] = (size_t) index;

                    p_buf += byteLen;
                    ++count;
                }
            }
            break;
        }
    }
    app.quit();
    return result;
}

nfdresult_t NFD_OpenDialog( const char *filterList, const nfdchar_t *defaultPath, nfdchar_t **outPath )
{
    return NFD_QTOpenDialog(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, nullptr, filterList, nullptr, defaultPath, outPath, nullptr);
}

nfdresult_t NFD_OpenDialogMultiple( const nfdchar_t *filterList, const nfdchar_t *defaultPath, nfdpathset_t *outPaths )
{
    return NFD_QTOpenDialog(QFileDialog::AcceptOpen, QFileDialog::ExistingFiles, nullptr, filterList, nullptr, defaultPath, nullptr, outPaths);
}

nfdresult_t NFD_SaveDialog( const nfdchar_t *filterList, const nfdchar_t *defaultPath, const nfdchar_t *defaultFilename, nfdchar_t **outPath )
{
    return NFD_QTOpenDialog(QFileDialog::AcceptSave, QFileDialog::ExistingFile, nullptr, filterList, nullptr, defaultPath, outPath, nullptr);
}

nfdresult_t NFD_ChooseDirectory( const nfdchar_t *prompt, const nfdchar_t *defaultPath, nfdchar_t **outPath )
{
    return NFD_QTOpenDialog(QFileDialog::AcceptOpen, QFileDialog::Directory, prompt, nullptr, nullptr, defaultPath, outPath, nullptr);
}
