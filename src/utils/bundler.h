

#ifndef BUNDLER_H_
#define BUNDLER_H_

class Bundler {
public:
	virtual ~Bundler() {}
	virtual bool saveAsAux(const QString &filename) = 0;
	virtual bool loadBundledAux(QDir &dir, QList<class ModelPart*> mps) {Q_UNUSED(dir); Q_UNUSED(mps); return true;};
	virtual bool preloadBundledAux(QDir &dir, bool dontAsk) {Q_UNUSED(dir); Q_UNUSED(dontAsk); return true;};
};

#endif /* BUNDLER_H_ */
