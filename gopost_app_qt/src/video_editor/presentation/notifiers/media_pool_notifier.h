#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <optional>
#include <vector>

#include "video_editor/domain/models/media_asset.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------
enum class MediaPoolViewMode { Grid, List };
enum class MediaPoolSortBy { Name, Date, Type, Duration, Size };

// ---------------------------------------------------------------------------
// MediaPoolState
// ---------------------------------------------------------------------------
struct MediaPoolState {
    std::vector<MediaAsset> assets;
    std::vector<MediaBin>   bins;
    QString searchQuery;
    std::optional<MediaAssetType> filterType; // nullopt = show all
    MediaPoolSortBy sortBy    = MediaPoolSortBy::Date;
    bool sortAscending        = false;
    MediaPoolViewMode viewMode = MediaPoolViewMode::Grid;
    std::optional<QString> activeBinId;

    /// Filtered + sorted assets (computed).
    std::vector<const MediaAsset*> filteredAssets() const;
};

// ---------------------------------------------------------------------------
// MediaPoolNotifier -- media bin/asset management for the editor
//
// Converted 1:1 from media_pool_notifier.dart.
// ---------------------------------------------------------------------------
class MediaPoolNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(int  assetCount READ assetCount NOTIFY stateChanged)
    Q_PROPERTY(int  viewMode   READ viewModeInt NOTIFY stateChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery NOTIFY stateChanged)
    Q_PROPERTY(QVariantList filteredAssets READ filteredAssetsVariant NOTIFY stateChanged)
    Q_PROPERTY(QString currentFilter READ currentFilter NOTIFY stateChanged)
    Q_PROPERTY(QString binBreadcrumb READ binBreadcrumb NOTIFY stateChanged)
    Q_PROPERTY(QString activeBinId   READ activeBinId   NOTIFY stateChanged)
    Q_PROPERTY(QVariantList bins     READ binsVariant    NOTIFY stateChanged)
    Q_PROPERTY(int selectedAssetCount READ selectedAssetCount NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedAssetIds READ selectedAssetIdsVariant NOTIFY selectionChanged)
    Q_PROPERTY(int selectionGeneration READ selectionGeneration NOTIFY selectionChanged)

public:
    explicit MediaPoolNotifier(QObject* parent = nullptr);
    ~MediaPoolNotifier() override;

    // Property accessors
    int     assetCount()  const { return static_cast<int>(state_.assets.size()); }
    int     viewModeInt() const { return static_cast<int>(state_.viewMode); }
    QString searchQuery() const { return state_.searchQuery; }
    QVariantList filteredAssetsVariant() const;
    QString currentFilter() const;

    const MediaPoolState& state() const { return state_; }

    // ---- assets -----------------------------------------------------------
    Q_INVOKABLE void importFile(const QString& path);
    Q_INVOKABLE void importFiles(const QVariantList& urls);
    Q_INVOKABLE void removeAsset(const QString& assetId);
    Q_INVOKABLE void moveAssetToBin(const QString& assetId, const QString& binId);
    Q_INVOKABLE void relinkAsset(const QString& assetId, const QString& newPath);
    Q_INVOKABLE void checkAllAssetsOnline();
    Q_INVOKABLE void selectAsset(const QString& assetId);
    Q_INVOKABLE void toggleAssetSelection(const QString& assetId);
    Q_INVOKABLE void rangeSelectAsset(const QString& assetId);
    Q_INVOKABLE void selectAllInBin();
    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE bool isAssetSelected(const QString& assetId) const;
    int selectedAssetCount() const { return selectedAssetIds_.size(); }
    int selectionGeneration() const { return selectionGeneration_; }
    QVariantList selectedAssetIdsVariant() const;
    Q_INVOKABLE void addToTimeline(const QString& assetId);
    Q_INVOKABLE void showFilePicker();
    Q_INVOKABLE void importFolder(const QString& path);
    Q_INVOKABLE void renameAsset(const QString& assetId, const QString& newName);
    Q_INVOKABLE void revealInExplorer(const QString& assetId);
    Q_INVOKABLE void navigateIntoBin(const QString& binId);
    Q_INVOKABLE void navigateUp();
    QString binBreadcrumb() const;
    QString activeBinId() const;
    QVariantList binsVariant() const;

    // ---- bins -------------------------------------------------------------
    Q_INVOKABLE void createBin(const QString& name);
    Q_INVOKABLE void renameBin(const QString& binId, const QString& name);
    Q_INVOKABLE void removeBin(const QString& binId);
    Q_INVOKABLE void setActiveBin(const QString& binId);

    // ---- search / filter / sort -------------------------------------------
    Q_INVOKABLE void setSearchQuery(const QString& query);
    Q_INVOKABLE void setFilter(const QString& filter);
    Q_INVOKABLE void setFilterType(int type);
    Q_INVOKABLE void clearFilterType();
    Q_INVOKABLE void setSortBy(int sortBy);
    Q_INVOKABLE void cycleSort();
    Q_INVOKABLE void toggleSortOrder();
    Q_INVOKABLE void setViewMode(int mode);

    // ---- query -------------------------------------------------------------
    Q_INVOKABLE double durationForPath(const QString& path) const;
    Q_INVOKABLE bool validateBatchSize(int count) const;

    // ---- persistence -------------------------------------------------------
    Q_INVOKABLE void loadFromData(const QVariantList& data);
    Q_INVOKABLE QVariantList toData() const;

signals:
    void stateChanged();
    void selectionChanged();
    void filePickerRequested();
    void addToTimelineRequested(const QString& assetId, const QString& filePath,
                                const QString& mediaType, const QString& displayName,
                                double duration);
    void importRejected(const QString& fileName, const QString& reason);
    void duplicateDetected(const QString& fileName);
    void batchTooLarge(int count, int maxAllowed) const;
    void importBatchCompleted(int succeeded, int failed, const QStringList& failedNames);

private:
    MediaPoolState state_;
    QString selectedAssetId_;             // last-clicked asset (for range selection anchor)
    QSet<QString> selectedAssetIds_;      // multi-select set
    QMap<QString, QSet<QString>> binSelections_;  // per-bin selection preservation
    int selectionGeneration_ = 0;
    QString generateAssetId() const;
    void probeMediaMetadata(MediaAsset& asset);
};

} // namespace gopost::video_editor
